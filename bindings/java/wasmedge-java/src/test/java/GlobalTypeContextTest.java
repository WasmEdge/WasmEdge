import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.GlobalTypeContext;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.WasmEdgeMutability;

public class GlobalTypeContextTest extends BaseTest {
    @Test
    public void testCreation() {
        ValueType valueType = ValueType.f32;
        WasmEdgeMutability mutability = WasmEdgeMutability.VAR;
        GlobalTypeContext globalTypeContext = new GlobalTypeContext(valueType, mutability);

        Assert.assertEquals(globalTypeContext.getValueType(), valueType);
        Assert.assertEquals(globalTypeContext.getMutability(), mutability);
    }
}
