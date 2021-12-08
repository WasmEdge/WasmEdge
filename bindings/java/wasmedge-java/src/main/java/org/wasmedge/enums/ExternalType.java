package org.wasmedge.enums;

public enum ExternalType {
    FUNCTION(0x00),
    TABLE(0x01),
    MEMORY(0x02),
    GLOBAL(0x03);

    private int val;

    private ExternalType(int val) {
        this.val = val;
    }

}
