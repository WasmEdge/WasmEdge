import org.junit.Test;
import org.wasmedge.CompilerContext;

public class CompilerContextTest extends BaseTest {

    @Test
    public void testCompile() {
        CompilerContext compilerContext = new CompilerContext();
        compilerContext.compile(WASM_PATH, WASM_PATH + ".so");
        compilerContext.delete();
    }
}
