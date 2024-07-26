package org.wasmedge.example_app

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import org.wasmedge.native_lib.NativeLib
import java.util.*

class MainActivity : AppCompatActivity() {

    lateinit var lib: NativeLib

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val tv = findViewById<TextView>(R.id.tv_text)

        lib = NativeLib(this)

        Thread {
            val lines = Vector<String>()
            val idxArr = arrayOf(20, 25, 28, 30, 32)
            for (idx: Int in idxArr) {
                lines.add("running fib(${idx}) ...")
                runOnUiThread {
                    tv.text = lines.joinToString("\n")
                }
                val begin = System.currentTimeMillis()
                val retVal = lib.wasmFibonacci(idx)
                val end = System.currentTimeMillis()
                lines.removeLast()
                lines.add("fib(${idx}) -> ${retVal}, ${end - begin}ms")
                runOnUiThread {
                    tv.text = lines.joinToString("\n")
                }
            }
        }.start()
    }
}
