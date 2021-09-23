package org.wasmedge;

public class WasmEdgeVM {

    static {
        System.loadLibrary("wasmedge_java");
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

}
