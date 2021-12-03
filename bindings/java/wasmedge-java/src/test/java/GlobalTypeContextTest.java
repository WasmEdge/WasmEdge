import org.junit.Test;
import org.wasmedge.GlobalTypeContext;

public class GlobalTypeContextTest extends BaseTest {
    @Test
    public void testCreation() {
        GlobalTypeContext globalTypeContext = new GlobalTypeContext(null, null);
    }
}
