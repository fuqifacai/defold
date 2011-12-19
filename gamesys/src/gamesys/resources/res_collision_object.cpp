#include "res_collision_object.h"

#include <string.h>

#include <dlib/hash.h>
#include <dlib/log.h>
#include "../gamesys.h"

namespace dmGameSystem
{

    static dmPhysics::HCollisionShape2D Create2DShape(const dmPhysicsDDF::CollisionShape* collision_shape, uint32_t shape_index)
    {
        const dmPhysicsDDF::CollisionShape::Shape* shape = &collision_shape->m_Shapes[shape_index];

        const float* data = collision_shape->m_Data.m_Data;
        uint32_t data_count = collision_shape->m_Data.m_Count;
        dmPhysics::HCollisionShape2D ret = 0;

        switch (shape->m_ShapeType)
        {
        case dmPhysicsDDF::CollisionShape::TYPE_SPHERE:
            if (shape->m_Index + 1 > data_count)
            {
                goto range_error;
            }
            ret = dmPhysics::NewCircleShape2D(data[shape->m_Index]);
            break;

        case dmPhysicsDDF::CollisionShape::TYPE_BOX:
            if (shape->m_Index + 3 > data_count)
            {
                goto range_error;
            }
            ret = dmPhysics::NewBoxShape2D(Vectormath::Aos::Vector3(data[shape->m_Index], data[shape->m_Index+1], data[shape->m_Index+2]));
            break;

        case dmPhysicsDDF::CollisionShape::TYPE_CAPSULE:
            // TODO: Add support
            dmLogError("%s", "Capsules are not supported in 2D.");
            break;

        case dmPhysicsDDF::CollisionShape::TYPE_HULL:
        {
            if (shape->m_Index + shape->m_Count > data_count)
            {
                goto range_error;
            }

            const uint32_t data_size = 2 * shape->m_Count / 3;
            float* data_2d = new float[2 * shape->m_Count / 3];
            for (uint32_t i = 0; i < data_size; ++i)
            {
                data_2d[i] = collision_shape->m_Data[shape->m_Index + i/2*3 + i%2];
            }
            ret = dmPhysics::NewPolygonShape2D(data_2d, data_size/2);
            delete [] data_2d;
        }
        break;

        default:
            // NOTE: We do not create hulls here. Hulls can't currently be created as embedded shapes
            // In the future we might support that or support the collision shape message as a external resource
            dmLogError("Unknown or unsupported shape type: %d", shape->m_ShapeType);
            break;
        }

        return ret;

range_error:
        dmLogError("Index out of range to shape data for shape index %d", shape_index);
        return 0;
    }

    static dmPhysics::HCollisionShape2D Create3DShape(const dmPhysicsDDF::CollisionShape* collision_shape, uint32_t shape_index)
    {
        const dmPhysicsDDF::CollisionShape::Shape* shape = &collision_shape->m_Shapes[shape_index];

        const float* data = collision_shape->m_Data.m_Data;
        uint32_t data_count = collision_shape->m_Data.m_Count;
        dmPhysics::HCollisionShape3D ret = 0;

        switch (shape->m_ShapeType)
        {
        case dmPhysicsDDF::CollisionShape::TYPE_SPHERE:
            if (shape->m_Index + 1 > data_count) {
                goto range_error;
            }
            ret = dmPhysics::NewSphereShape3D(data[shape->m_Index]);
            break;

        case dmPhysicsDDF::CollisionShape::TYPE_BOX:
            if (shape->m_Index + 3 > data_count) {
                goto range_error;
            }
            ret = dmPhysics::NewBoxShape3D(Vectormath::Aos::Vector3(data[shape->m_Index], data[shape->m_Index+1], data[shape->m_Index+2]));
            break;

        case dmPhysicsDDF::CollisionShape::TYPE_CAPSULE:
            if (shape->m_Index + 2 > data_count) {
                goto range_error;
            }
            ret = dmPhysics::NewCapsuleShape3D(data[shape->m_Index], data[shape->m_Index+1]);
            break;

        case dmPhysicsDDF::CollisionShape::TYPE_HULL:
            if (shape->m_Index + shape->m_Count > data_count)
            {
                goto range_error;
            }
            ret = dmPhysics::NewConvexHullShape3D(&collision_shape->m_Data[shape->m_Index], shape->m_Count);
            break;

        default:
            // NOTE: We do not create hulls here. Hulls can't currently be created as embedded shapes
            // In the future we might support that or support the collision shape message as a external resource
            dmLogError("Unknown or unsupported shape type: %d", shape->m_ShapeType);
            break;
        }

        return ret;

range_error:
        dmLogError("Index out of range to shape data for shape index %d", shape_index);
        return 0;
    }

    bool AcquireResources(PhysicsContext* physics_context, dmResource::HFactory factory, const void* buffer, uint32_t buffer_size,
        CollisionObjectResource* resource, const char* filename)
    {
        dmDDF::Result e = dmDDF::LoadMessage<dmPhysicsDDF::CollisionObjectDesc>(buffer, buffer_size, &resource->m_DDF);
        if ( e != dmDDF::RESULT_OK )
        {
            return false;
        }
        resource->m_Group = dmHashString64(resource->m_DDF->m_Group);
        uint32_t mask_count = resource->m_DDF->m_Mask.m_Count;
        if (mask_count > 16)
        {
            dmLogWarning("The collision object '%s' has a collision mask containing more than 16 groups, the rest will be ignored.", filename);
            mask_count = 16;
        }
        for (uint32_t i = 0; i < mask_count; ++i)
        {
            resource->m_Mask[i] = dmHashString64(resource->m_DDF->m_Mask[i]);
        }

        if (resource->m_DDF->m_CollisionShape && resource->m_DDF->m_CollisionShape[0] != '\0')
        {
            void* res;
            dmResource::FactoryResult factory_result = dmResource::Get(factory, resource->m_DDF->m_CollisionShape, &res);
            if (factory_result == dmResource::FACTORY_RESULT_OK)
            {
                uint32_t tile_grid_type;
                factory_result = dmResource::GetTypeFromExtension(factory, "tilegridc", &tile_grid_type);
                if (factory_result == dmResource::FACTORY_RESULT_OK)
                {
                    uint32_t res_type;
                    factory_result = dmResource::GetType(factory, res, &res_type);
                    if (factory_result == dmResource::FACTORY_RESULT_OK && res_type == tile_grid_type)
                    {
                        resource->m_TileGridResource = (TileGridResource*)res;
                        resource->m_TileGrid = 1;
                        // Add the tile grid as the first and only shape
                        resource->m_Shapes2D[0] = resource->m_TileGridResource->m_GridShape;
                        resource->m_ShapeCount++;
                        return true;
                    }
                    // NOTE: Fall-trough "by design" here, ie not "else return false"
                    // The control flow should be improved. Currently very difficult to follow or understand
                    // the expected logical paths
                }
            }
        }

        const dmPhysicsDDF::CollisionShape* embedded_shape = &resource->m_DDF->m_EmbeddedCollisionShape;
        const dmPhysicsDDF::CollisionShape::Shape* shapes = embedded_shape->m_Shapes.m_Data;
        if (shapes)
        {
            // Create embedded convex shapes
            uint32_t count = embedded_shape->m_Shapes.m_Count;
            if (count > COLLISION_OBJECT_MAX_SHAPES)
            {
                dmLogWarning("Too many shapes in collision object. Up to %d is supported (%d). Discarding overflowing shapes.", COLLISION_OBJECT_MAX_SHAPES, count);
                count = COLLISION_OBJECT_MAX_SHAPES;
            }

            uint32_t current_shape_count = resource->m_ShapeCount;
            for (uint32_t i = 0; i < count; ++i)
            {
                if (physics_context->m_3D)
                {
                    dmPhysics::HCollisionObject3D shape = Create3DShape(embedded_shape, i);
                    if (shape)
                    {
                        resource->m_Shapes3D[current_shape_count] = shape;
                        resource->m_ShapeTranslation[current_shape_count] = Vectormath::Aos::Vector3(shapes[i].m_Position);
                        resource->m_ShapeRotation[current_shape_count] = shapes[i].m_Rotation;
                        current_shape_count++;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    dmPhysics::HCollisionObject2D shape = Create2DShape(embedded_shape, i);
                    if (shape)
                    {
                        resource->m_Shapes2D[current_shape_count] = shape;
                        resource->m_ShapeTranslation[current_shape_count] = Vectormath::Aos::Vector3(shapes[i].m_Position);
                        resource->m_ShapeRotation[current_shape_count] = shapes[i].m_Rotation;
                        current_shape_count++;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            resource->m_ShapeCount = current_shape_count;
            assert(resource->m_ShapeCount <= COLLISION_OBJECT_MAX_SHAPES);
            return true;
        }
        else
        {
            dmLogError("No shapes found in collision object");
            return false;
        }
    }

    void ReleaseResources(PhysicsContext* physics_context, dmResource::HFactory factory, CollisionObjectResource* resource)
    {
        if (resource->m_TileGrid)
        {
            if (resource->m_TileGridResource)
                dmResource::Release(factory, resource->m_TileGridResource);
        }
        else
        {
            uint32_t shape_count = resource->m_ShapeCount;
            for (uint32_t i = 0; i < shape_count; ++i)
            {
                if (physics_context->m_3D)
                {
                    dmPhysics::DeleteCollisionShape3D(resource->m_Shapes3D[i]);
                }
                else
                {
                    dmPhysics::DeleteCollisionShape2D(resource->m_Shapes2D[i]);
                }
            }
        }
        if (resource->m_DDF)
            dmDDF::FreeMessage(resource->m_DDF);
    }

    dmResource::CreateResult ResCollisionObjectCreate(dmResource::HFactory factory,
                                             void* context,
                                             const void* buffer, uint32_t buffer_size,
                                             dmResource::SResourceDescriptor* resource,
                                             const char* filename)
    {
        CollisionObjectResource* collision_object = new CollisionObjectResource();
        memset(collision_object, 0, sizeof(CollisionObjectResource));
        PhysicsContext* physics_context = (PhysicsContext*) context;
        if (AcquireResources(physics_context, factory, buffer, buffer_size, collision_object, filename))
        {
            resource->m_Resource = collision_object;
            return dmResource::CREATE_RESULT_OK;
        }
        else
        {
            ReleaseResources(physics_context, factory, collision_object);
            delete collision_object;
            return dmResource::CREATE_RESULT_FORMAT_ERROR;
        }
    }

    dmResource::CreateResult ResCollisionObjectDestroy(dmResource::HFactory factory,
                                              void* context,
                                              dmResource::SResourceDescriptor* resource)
    {
        CollisionObjectResource* collision_object = (CollisionObjectResource*)resource->m_Resource;
        PhysicsContext* physics_context = (PhysicsContext*) context;
        ReleaseResources(physics_context, factory, collision_object);
        delete collision_object;
        return dmResource::CREATE_RESULT_OK;
    }

    dmResource::CreateResult ResCollisionObjectRecreate(dmResource::HFactory factory,
                                                void* context,
                                                const void* buffer, uint32_t buffer_size,
                                                dmResource::SResourceDescriptor* resource,
                                                const char* filename)
    {
        CollisionObjectResource* collision_object = (CollisionObjectResource*)resource->m_Resource;
        CollisionObjectResource tmp_collision_object;
        memset(&tmp_collision_object, 0, sizeof(CollisionObjectResource));
        PhysicsContext* physics_context = (PhysicsContext*) context;
        if (AcquireResources(physics_context, factory, buffer, buffer_size, &tmp_collision_object, filename))
        {
            ReleaseResources(physics_context, factory, collision_object);
            *collision_object = tmp_collision_object;
            return dmResource::CREATE_RESULT_OK;
        }
        else
        {
            ReleaseResources(physics_context, factory, &tmp_collision_object);
            return dmResource::CREATE_RESULT_FORMAT_ERROR;
        }
    }
}
