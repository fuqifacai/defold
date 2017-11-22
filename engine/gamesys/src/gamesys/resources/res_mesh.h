#ifndef DM_GAMESYS_RES_MESH_H
#define DM_GAMESYS_RES_MESH_H

#include <stdint.h>

#include <resource/resource.h>
#include <render/render.h>
#include <graphics/graphics.h>
#include "mesh_ddf.h"
#include <dlib/buffer.h>

namespace dmGameSystem
{
    struct MeshResource
    {
        MeshResource() {
            m_Mesh = 0x0;
            m_Material = 0x0;
            m_VertexDeclaration = 0x0;
            m_VertexBuffer = 0x0;
        }
        dmMeshDDF::Mesh*        m_Mesh;
        dmRender::HMaterial     m_Material;
        dmGraphics::HTexture    m_Textures[dmRender::RenderObject::MAX_TEXTURE_COUNT];
        dmhash_t                m_TexturePaths[dmRender::RenderObject::MAX_TEXTURE_COUNT];

        dmGraphics::HVertexDeclaration m_VertexDeclaration;
        dmGraphics::HVertexBuffer      m_VertexBuffer;
    };

    dmResource::Result ResMeshPreload(const dmResource::ResourcePreloadParams& params);

    dmResource::Result ResMeshCreate(const dmResource::ResourceCreateParams& params);

    dmResource::Result ResMeshDestroy(const dmResource::ResourceDestroyParams& params);

    dmResource::Result ResMeshRecreate(const dmResource::ResourceRecreateParams& params);
}

#endif // DM_GAMESYS_RES_MESH_H