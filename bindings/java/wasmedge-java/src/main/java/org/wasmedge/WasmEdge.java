package org.wasmedge;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

/**
 * WasmeEdge class, for initializing WasmVM.
 */
public class WasmEdge {

    private static boolean loaded;
    private static final String NATIVE_LIBRARY_NAME = "wasmedge_jni";

    public static synchronized void init() {
        System.loadLibrary(NATIVE_LIBRARY_NAME);
    }

    /**
     * Try to load native libraries.
     */
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

        if (os.contains("linux")) {
            prefix = "lib";
            ext = "so";
        } else if (os.contains("mac os") || os.contains("darwin")) {
            prefix = "lib";
            ext = "dylib";
        } else if (os.contains("windows")) {
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
     * @return major version.
     */
    public native long getMajorVersion();

    /**
     * Get the minor version of underlying C API.
     *
     * @return minor version.
     */
    public native long getMinorVersion();

    /**
     * Get the patch version of underlying C API.
     *
     * @return patch version.
     */
    public native long getPatchVersion();

    /**
     * set log level.
     *
     * @param logLevel level.
     */
    public static void setLogLevel(LogLevel logLevel) {
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

    private static native void setErrorLevel();

    private static native void setDebugLevel();

    /**
     * Log level enum.
     */
    public enum LogLevel {
        DEBUG, ERROR
    }

}
