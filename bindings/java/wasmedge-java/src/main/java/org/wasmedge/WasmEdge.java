package org.wasmedge;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Properties;

/**
 * WasmEdge class, for initializing WasmVM.
 */
public class WasmEdge {

    private static boolean loaded;
    private static final String NATIVE_LIBRARY_NAME = "wasmedge_jni";
    private static final String PROPERTIES_FILE = "wasmedge-java.properties";
    private static final String JNI_LIB_VERSION = "jnilib.version";

    public static synchronized void init() {
        load();
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

    private static String getLibVersion() throws IOException {
        final Properties props;
        try (InputStream in = WasmEdge.class.getResourceAsStream('/' + PROPERTIES_FILE)) {
            props = new Properties();
            props.load(in);
        }
        return props.getProperty(JNI_LIB_VERSION);
    }

    private static String libraryPath() throws IOException {
        String ext = "";
        String prefix = "";
        String os = System.getProperty("os.name").toLowerCase();
        String osType = "";

        if (os.contains("linux")) {
            prefix = "lib";
            ext = "so";
            osType = "linux";
        } else if (os.contains("mac os") || os.contains("darwin")) {
            prefix = "lib";
            ext = "dylib";
            osType = "mac";
        } else if (os.contains("windows")) {
            ext = "dll";
            osType = "windows";
        }

        String fileName = String.format("%s%s_%s_%s", prefix,
            NATIVE_LIBRARY_NAME, getLibVersion(), osType);
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
