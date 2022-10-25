package org.wasmedge;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

public class WasmEdge {

    private static boolean loaded;
    private static final String NATIVE_LIBRARY_NAME ="wasmedge_jni";

    public static synchronized void init() {
        System.loadLibrary(NATIVE_LIBRARY_NAME);
    }

    public static synchronized void load() {
        if (loaded) {
            return;
        }
        if (tryLoadFromLibraryPath()) {
            loaded = true;
            return;
        }

        final String libraryPath;
        try {
            libraryPath = libraryPath();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        System.load(libraryPath);
        loaded = true;
    }

    private static boolean tryLoadFromLibraryPath() {
        try {
            System.loadLibrary(NATIVE_LIBRARY_NAME);
        } catch (UnsatisfiedLinkError ignored) {
            return false;
        }
        return true;
    }

    private static String libraryPath() throws IOException {
        String ext = "";
        String prefix = "";
        String os = System.getProperty("os.name").toLowerCase();

        if(os.contains("linux")) {
            prefix = "lib";
            ext = "so";
        } else if(os.contains("mac os") || os.contains("darwin")) {
            prefix = "lib";
            ext = "dylib";
        } else if(os.contains("windows")) {
            ext = "dll";
        }

        String fileName = prefix + NATIVE_LIBRARY_NAME;
        Path tempFile = Files.createTempFile(fileName, ext);
        try (InputStream in = WasmEdge.class.getResourceAsStream('/' + fileName + ext)) {
            Files.copy(in, tempFile, StandardCopyOption.REPLACE_EXISTING);
        }
        return tempFile.toString();
    }

    /**
     * Get version string of underlying C API.
     *
     * @return version
     */
    public native String getVersion();

    /**
     * Get the major version of underlying C API.
     *
     * @return
     */
    public native long getMajorVersion();

    /**
     * Get the minor version of underlying C API.
     *
     * @return
     */
    public native long getMinorVersion();

    /**
     * Get the patch version of underlying C API.
     *
     * @return
     */
    public native long getPatchVersion();

    /**
     * @param logLevel
     */
    public void setLogLevel(LogLevel logLevel) {
        switch (logLevel) {
            case ERROR:
                setErrorLevel();
                return;
            case DEBUG:
                setDebugLevel();
                return;
            default:
                throw new RuntimeException("Invalid log level " + logLevel);
        }

    }

    private native void setErrorLevel();

    private native void setDebugLevel();

    public enum LogLevel {
        DEBUG,
        ERROR
    }

}
