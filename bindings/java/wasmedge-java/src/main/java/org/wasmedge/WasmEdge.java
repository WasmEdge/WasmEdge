package org.wasmedge;

public class WasmEdge {

    static {
        System.loadLibrary("wasmedge_java");
    }

    enum LogLevel {
        DEBUG,
        ERROR;
    }

    /**
     * Get version string of underlying C API.
     * @return version
     */
    public native String getVersion();

    /**
     * Get the major version of underlying C API.
     * @return
     */
    public native long getMajorVersion();

    /**
     * Get the minor version of underlying C API.
     * @return
     */
    public native long getMinorVersion();

    /**
     * Get the patch version of underlying C API.
     * @return
     */
    public native long getPatchVersion();

    /**
     *
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

}
