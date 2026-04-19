;; mandelbrot_bench.wat
;; WAT port of mandelbrot-set-in-threads/mandelbrot.c (single-threaded core).
;;
;; Includes: colour, iterateEquation, scale, and a main() that calls
;; iterateEquation with fully constant arguments so the execution path is
;; deterministic.
;;
;; Constant-folding analysis:
;;   Every arithmetic sub-expression in every function combines at least one
;;   local.get with a constant, so NO const+const+binop triple appears.
;;   Specifically:
;;     iterateEquation:  f64.const 4.0 and f64.const 2.0 are always preceded
;;                       by f64.mul results (variables), not other constants.
;;     colour:           i32.const 1024, 256, 512, 255 are all adjacent to
;;                       local.get values.
;;     scale:            pure variable arithmetic; no manifest constants at all.
;;     main:             three push-constants followed immediately by `call`,
;;                       not a binary op; the triple never forms.
;;
;; Expected optimizeConstantExpressions result:
;;   0 nops inserted, 0% instruction reduction.
;;   This mirrors what a C compiler (clang -O1) produces: all intra-expression
;;   constants are already folded at the source level, leaving only
;;   constant-variable mixed arithmetic in the Wasm bytecode.

(module
  ;; colour(iteration, offset, scale) -> i32
  ;; Implements:
  ;;   iteration = ((iteration * scale) + offset) % 1024
  ;;   if iteration < 256  → return iteration
  ;;   if iteration < 512  → return 255 - (iteration - 255)
  ;;   else                → return 0
  (func $colour (param $iter i32) (param $offset i32) (param $scale i32) (result i32)
    ;; iteration = ((iteration * scale) + offset) % 1024
    local.get $iter
    local.get $scale
    i32.mul
    local.get $offset
    i32.add
    i32.const 1024
    i32.rem_s
    local.set $iter

    ;; if (iteration < 256) return iteration
    block $done (result i32)
      block $not_low
        local.get $iter
        i32.const 256
        i32.lt_s
        i32.eqz
        br_if $not_low
        local.get $iter
        br $done
      end ;; $not_low

      ;; if (iteration < 512) return 255 - (iteration - 255)
      block $not_mid
        local.get $iter
        i32.const 512
        i32.lt_s
        i32.eqz
        br_if $not_mid
        i32.const 255
        local.get $iter
        i32.const 255
        i32.sub
        i32.sub
        br $done
      end ;; $not_mid

      ;; else return 0
      i32.const 0
    end ;; $done
  )

  ;; iterateEquation(x0, y0, maxIterations) -> i32
  ;; Implements the Mandelbrot escape-time algorithm:
  ;;   a=0, b=0, rx=0, ry=0, iterations=0
  ;;   while iterations < maxIterations && (rx*rx + ry*ry) <= 4.0:
  ;;     rx = a*a - b*b + x0
  ;;     ry = 2.0*a*b + y0
  ;;     a = rx; b = ry; iterations++
  ;;   return iterations
  (func $iterateEquation
        (param $x0 f64) (param $y0 f64) (param $maxIter i32)
        (result i32)
    (local $a f64) (local $b f64) (local $rx f64) (local $ry f64)
    (local $iter i32)
    ;; locals default-initialized to 0 / 0.0

    block $break
      loop $loop
        ;; if iter >= maxIter: break
        local.get $iter
        local.get $maxIter
        i32.ge_s
        br_if $break

        ;; if rx*rx + ry*ry > 4.0: break
        local.get $rx
        local.get $rx
        f64.mul
        local.get $ry
        local.get $ry
        f64.mul
        f64.add
        f64.const 4.0
        f64.gt
        br_if $break

        ;; rx = a*a - b*b + x0
        local.get $a
        local.get $a
        f64.mul
        local.get $b
        local.get $b
        f64.mul
        f64.sub
        local.get $x0
        f64.add
        local.set $rx

        ;; ry = 2.0*a*b + y0
        f64.const 2.0
        local.get $a
        f64.mul
        local.get $b
        f64.mul
        local.get $y0
        f64.add
        local.set $ry

        ;; a = rx; b = ry
        local.get $rx
        local.set $a
        local.get $ry
        local.set $b

        ;; iter++
        local.get $iter
        i32.const 1
        i32.add
        local.set $iter

        br $loop
      end ;; $loop
    end ;; $break

    local.get $iter
  )

  ;; scale(domainStart, domainLength, screenLength, step) -> f64
  ;; Implements: domainStart + domainLength * (step - screenLength) / screenLength
  (func $scale
        (param $domainStart f64) (param $domainLength f64)
        (param $screenLength i32) (param $step i32)
        (result f64)
    local.get $domainStart
    local.get $domainLength
    local.get $step
    local.get $screenLength
    i32.sub
    f64.convert_i32_s
    local.get $screenLength
    f64.convert_i32_s
    f64.div
    f64.mul
    f64.add
  )

  ;; main: call iterateEquation with fully constant arguments.
  ;;   iterateEquation(-0.5, 0.0, 100)
  ;; The point (-0.5, 0.0) is inside the Mandelbrot set, so the
  ;; iteration count equals maxIterations = 100.
  (func (export "main") (result i32)
    f64.const -0.5
    f64.const 0.0
    i32.const 100
    call $iterateEquation
  )
)
