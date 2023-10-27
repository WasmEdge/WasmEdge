package org.wasmedge;

import java.io.File;
import java.io.InputStream;
import java.nio.file.FileSystemNotFoundException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.ProviderNotFoundException;
import java.nio.file.StandardCopyOption;

public class NativeUtils {

    public static final String NATIVE_FOLDER_PATH_PREFIX = "wasmedge_jni_tmp";

    private static File temporaryDir;

    private NativeUtils() {
    }

    /**
     * Loads library from current JAR archive
     * 
     * The file from JAR is copied into system temporary directory and then loaded.
     * The temporary file is deleted after
     * exiting.
     * Method uses String as filename because the pathname is "abstract", not
     * system-dependent.
     * 
     * @param libName The name of the lib, e.g. wasmedge_jni
     */
    public static void loadLibraryFromJar(String libName) {
        String filename = System.mapLibraryName(libName);
        String path = String.format("/%s", filename);

        if (temporaryDir == null) {
            temporaryDir = createTempDirectory(NATIVE_FOLDER_PATH_PREFIX);
            temporaryDir.deleteOnExit();
        }

        File temp = new File(temporaryDir, filename);

        try (InputStream is = NativeUtils.class.getResourceAsStream(path)) {
            Files.copy(is, temp.toPath(), StandardCopyOption.REPLACE_EXISTING);
        } catch (Exception e) {
            temp.delete();
            throw new RuntimeException(e);
        }

        try {
            System.load(temp.getAbsolutePath());
        } finally {
            if (isPosixCompliant()) {
                // Assume POSIX compliant file system, can be deleted after loading
                temp.delete();
            } else {
                // Assume non-POSIX, and don't delete until last file descriptor closed
                temp.deleteOnExit();
            }
        }
    }

    private static boolean isPosixCompliant() {
        try {
            return FileSystems.getDefault()
                    .supportedFileAttributeViews()
                    .contains("posix");
        } catch (FileSystemNotFoundException
                | ProviderNotFoundException
                | SecurityException e) {
            return false;
        }
    }

    private static File createTempDirectory(String prefix) {
        String tempDir = System.getProperty("java.io.tmpdir");
        File generatedDir = new File(tempDir, prefix + System.nanoTime());

        if (!generatedDir.mkdir())
            throw new RuntimeException("Failed to create temp directory " + generatedDir.getName());

        return generatedDir;
    }
}
