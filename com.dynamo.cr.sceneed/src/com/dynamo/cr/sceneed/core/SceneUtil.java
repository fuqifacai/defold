package com.dynamo.cr.sceneed.core;

import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;

import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.preference.IPreferenceStore;

import com.dynamo.cr.sceneed.Activator;
import com.dynamo.cr.sceneed.ui.preferences.PreferenceConstants;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;

public class SceneUtil {
    public static void saveMessage(Message message, OutputStream contents, IProgressMonitor monitor) throws IOException, CoreException {
        OutputStreamWriter writer = new OutputStreamWriter(contents);
        try {
            TextFormat.print(message, writer);
            writer.flush();
        } finally {
            writer.close();
        }
    }

    private static Boolean isMac = null;
    public static boolean isMac() {
        if (isMac == null) {
            isMac = System.getProperty("os.name").toLowerCase().indexOf("mac") >= 0;
        }
        return isMac;
    }

    public enum MouseType {
        ONE_BUTTON,
        THREE_BUTTON
    }

    public static MouseType getMouseType() {
        IPreferenceStore store = Activator.getDefault().getPreferenceStore();
        String typeValue = store.getString(PreferenceConstants.P_MOUSE_TYPE);
        return MouseType.valueOf(typeValue);
    }
}
