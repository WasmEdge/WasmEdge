```
Help on module WasmEdge:

NAME
    WasmEdge

CLASSES
    Boost.Python.enum(builtins.int)
        Host
        Proposal
    Boost.Python.instance(builtins.object)
        Configure
        Logging
        Result
        Store
        VM
    
    class Configure(Boost.Python.instance)
     |  Method resolution order:
     |      Configure
     |      Boost.Python.instance
     |      builtins.object
     |  
     |  Static methods defined here:
     |  
     |  __init__(...)
     |      __init__( (object)arg1) -> None :
     |      
     |          C++ signature :
     |              void __init__(_object*)
     |  
     |  __reduce__ = <unnamed Boost.Python function>(...)
     |  
     |  add(...)
     |      add( (Configure)arg1, (Proposal)arg2) -> None :
     |      
     |          C++ signature :
     |              void add(pysdk::Configure {lvalue},WasmEdge_Proposal)
     |      
     |      add( (Configure)arg1, (Host)arg2) -> None :
     |      
     |          C++ signature :
     |              void add(pysdk::Configure {lvalue},WasmEdge_HostRegistration)
     |  
     |  remove(...)
     |      remove( (Configure)arg1, (Proposal)arg2) -> None :
     |      
     |          C++ signature :
     |              void remove(pysdk::Configure {lvalue},WasmEdge_Proposal)
     |      
     |      remove( (Configure)arg1, (Host)arg2) -> None :
     |      
     |          C++ signature :
     |              void remove(pysdk::Configure {lvalue},WasmEdge_HostRegistration)
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  __instance_size__ = 24
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from Boost.Python.instance:
     |  
     |  __new__(*args, **kwargs) from Boost.Python.class
     |      Create and return a new object.  See help(type) for accurate signature.
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from Boost.Python.instance:
     |  
     |  __dict__
     |  
     |  __weakref__
    
    class Host(Boost.Python.enum)
     |  int([x]) -> integer
     |  int(x, base=10) -> integer
     |  
     |  Convert a number or string to an integer, or return 0 if no arguments
     |  are given.  If x is a number, return x.__int__().  For floating point
     |  numbers, this truncates towards zero.
     |  
     |  If x is not a number or if base is given, then x must be a string,
     |  bytes, or bytearray instance representing an integer literal in the
     |  given base.  The literal can be preceded by '+' or '-' and be surrounded
     |  by whitespace.  The base defaults to 10.  Valid bases are 0 and 2-36.
     |  Base 0 means to interpret the base from the string as an integer literal.
     |  >>> int('0b100', base=0)
     |  4
     |  
     |  Method resolution order:
     |      Host
     |      Boost.Python.enum
     |      builtins.int
     |      builtins.object
     |  
     |  Data and other attributes defined here:
     |  
     |  Wasi = WasmEdge.Host.Wasi
     |  
     |  WasmEdge = WasmEdge.Host.WasmEdge
     |  
     |  names = {'Wasi': WasmEdge.Host.Wasi, 'WasmEdge': WasmEdge.Host.WasmEdg...
     |  
     |  values = {0: WasmEdge.Host.Wasi, 1: WasmEdge.Host.WasmEdge}
     |  
     |  ----------------------------------------------------------------------
     |  Methods inherited from Boost.Python.enum:
     |  
     |  __repr__(self, /)
     |      Return repr(self).
     |  
     |  __str__(self, /)
     |      Return str(self).
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from Boost.Python.enum:
     |  
     |  name
     |  
     |  ----------------------------------------------------------------------
     |  Methods inherited from builtins.int:
     |  
     |  __abs__(self, /)
     |      abs(self)
     |  
     |  __add__(self, value, /)
     |      Return self+value.
     |  
     |  __and__(self, value, /)
     |      Return self&value.
     |  
     |  __bool__(self, /)
     |      self != 0
     |  
     |  __ceil__(...)
     |      Ceiling of an Integral returns itself.
     |  
     |  __divmod__(self, value, /)
     |      Return divmod(self, value).
     |  
     |  __eq__(self, value, /)
     |      Return self==value.
     |  
     |  __float__(self, /)
     |      float(self)
     |  
     |  __floor__(...)
     |      Flooring an Integral returns itself.
     |  
     |  __floordiv__(self, value, /)
     |      Return self//value.
     |  
     |  __format__(self, format_spec, /)
     |      Default object formatter.
     |  
     |  __ge__(self, value, /)
     |      Return self>=value.
     |  
     |  __getattribute__(self, name, /)
     |      Return getattr(self, name).
     |  
     |  __getnewargs__(self, /)
     |  
     |  __gt__(self, value, /)
     |      Return self>value.
     |  
     |  __hash__(self, /)
     |      Return hash(self).
     |  
     |  __index__(self, /)
     |      Return self converted to an integer, if self is suitable for use as an index into a list.
     |  
     |  __int__(self, /)
     |      int(self)
     |  
     |  __invert__(self, /)
     |      ~self
     |  
     |  __le__(self, value, /)
     |      Return self<=value.
     |  
     |  __lshift__(self, value, /)
     |      Return self<<value.
     |  
     |  __lt__(self, value, /)
     |      Return self<value.
     |  
     |  __mod__(self, value, /)
     |      Return self%value.
     |  
     |  __mul__(self, value, /)
     |      Return self*value.
     |  
     |  __ne__(self, value, /)
     |      Return self!=value.
     |  
     |  __neg__(self, /)
     |      -self
     |  
     |  __or__(self, value, /)
     |      Return self|value.
     |  
     |  __pos__(self, /)
     |      +self
     |  
     |  __pow__(self, value, mod=None, /)
     |      Return pow(self, value, mod).
     |  
     |  __radd__(self, value, /)
     |      Return value+self.
     |  
     |  __rand__(self, value, /)
     |      Return value&self.
     |  
     |  __rdivmod__(self, value, /)
     |      Return divmod(value, self).
     |  
     |  __rfloordiv__(self, value, /)
     |      Return value//self.
     |  
     |  __rlshift__(self, value, /)
     |      Return value<<self.
     |  
     |  __rmod__(self, value, /)
     |      Return value%self.
     |  
     |  __rmul__(self, value, /)
     |      Return value*self.
     |  
     |  __ror__(self, value, /)
     |      Return value|self.
     |  
     |  __round__(...)
     |      Rounding an Integral returns itself.
     |      Rounding with an ndigits argument also returns an integer.
     |  
     |  __rpow__(self, value, mod=None, /)
     |      Return pow(value, self, mod).
     |  
     |  __rrshift__(self, value, /)
     |      Return value>>self.
     |  
     |  __rshift__(self, value, /)
     |      Return self>>value.
     |  
     |  __rsub__(self, value, /)
     |      Return value-self.
     |  
     |  __rtruediv__(self, value, /)
     |      Return value/self.
     |  
     |  __rxor__(self, value, /)
     |      Return value^self.
     |  
     |  __sizeof__(self, /)
     |      Returns size in memory, in bytes.
     |  
     |  __sub__(self, value, /)
     |      Return self-value.
     |  
     |  __truediv__(self, value, /)
     |      Return self/value.
     |  
     |  __trunc__(...)
     |      Truncating an Integral returns itself.
     |  
     |  __xor__(self, value, /)
     |      Return self^value.
     |  
     |  as_integer_ratio(self, /)
     |      Return integer ratio.
     |      
     |      Return a pair of integers, whose ratio is exactly equal to the original int
     |      and with a positive denominator.
     |      
     |      >>> (10).as_integer_ratio()
     |      (10, 1)
     |      >>> (-10).as_integer_ratio()
     |      (-10, 1)
     |      >>> (0).as_integer_ratio()
     |      (0, 1)
     |  
     |  bit_length(self, /)
     |      Number of bits necessary to represent self in binary.
     |      
     |      >>> bin(37)
     |      '0b100101'
     |      >>> (37).bit_length()
     |      6
     |  
     |  conjugate(...)
     |      Returns self, the complex conjugate of any int.
     |  
     |  to_bytes(self, /, length, byteorder, *, signed=False)
     |      Return an array of bytes representing an integer.
     |      
     |      length
     |        Length of bytes object to use.  An OverflowError is raised if the
     |        integer is not representable with the given number of bytes.
     |      byteorder
     |        The byte order used to represent the integer.  If byteorder is 'big',
     |        the most significant byte is at the beginning of the byte array.  If
     |        byteorder is 'little', the most significant byte is at the end of the
     |        byte array.  To request the native byte order of the host system, use
     |        `sys.byteorder' as the byte order value.
     |      signed
     |        Determines whether two's complement is used to represent the integer.
     |        If signed is False and a negative integer is given, an OverflowError
     |        is raised.
     |  
     |  ----------------------------------------------------------------------
     |  Class methods inherited from builtins.int:
     |  
     |  from_bytes(bytes, byteorder, *, signed=False) from builtins.type
     |      Return the integer represented by the given array of bytes.
     |      
     |      bytes
     |        Holds the array of bytes to convert.  The argument must either
     |        support the buffer protocol or be an iterable object producing bytes.
     |        Bytes and bytearray are examples of built-in objects that support the
     |        buffer protocol.
     |      byteorder
     |        The byte order used to represent the integer.  If byteorder is 'big',
     |        the most significant byte is at the beginning of the byte array.  If
     |        byteorder is 'little', the most significant byte is at the end of the
     |        byte array.  To request the native byte order of the host system, use
     |        `sys.byteorder' as the byte order value.
     |      signed
     |        Indicates whether two's complement is used to represent the integer.
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from builtins.int:
     |  
     |  __new__(*args, **kwargs) from builtins.type
     |      Create and return a new object.  See help(type) for accurate signature.
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from builtins.int:
     |  
     |  denominator
     |      the denominator of a rational number in lowest terms
     |  
     |  imag
     |      the imaginary part of a complex number
     |  
     |  numerator
     |      the numerator of a rational number in lowest terms
     |  
     |  real
     |      the real part of a complex number
    
    class Logging(Boost.Python.instance)
     |  Method resolution order:
     |      Logging
     |      Boost.Python.instance
     |      builtins.object
     |  
     |  Static methods defined here:
     |  
     |  __init__(...)
     |      __init__( (object)arg1) -> None :
     |      
     |          C++ signature :
     |              void __init__(_object*)
     |  
     |  __reduce__ = <unnamed Boost.Python function>(...)
     |  
     |  __str__(...)
     |      __str__( (Logging)arg1) -> str :
     |      
     |          C++ signature :
     |              char const* __str__(pysdk::logging {lvalue})
     |  
     |  debug(...)
     |      debug() -> None :
     |      
     |          C++ signature :
     |              void debug()
     |  
     |  error(...)
     |      error() -> None :
     |      
     |          C++ signature :
     |              void error()
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  __instance_size__ = 24
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from Boost.Python.instance:
     |  
     |  __new__(*args, **kwargs) from Boost.Python.class
     |      Create and return a new object.  See help(type) for accurate signature.
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from Boost.Python.instance:
     |  
     |  __dict__
     |  
     |  __weakref__
    
    class Proposal(Boost.Python.enum)
     |  int([x]) -> integer
     |  int(x, base=10) -> integer
     |  
     |  Convert a number or string to an integer, or return 0 if no arguments
     |  are given.  If x is a number, return x.__int__().  For floating point
     |  numbers, this truncates towards zero.
     |  
     |  If x is not a number or if base is given, then x must be a string,
     |  bytes, or bytearray instance representing an integer literal in the
     |  given base.  The literal can be preceded by '+' or '-' and be surrounded
     |  by whitespace.  The base defaults to 10.  Valid bases are 0 and 2-36.
     |  Base 0 means to interpret the base from the string as an integer literal.
     |  >>> int('0b100', base=0)
     |  4
     |  
     |  Method resolution order:
     |      Proposal
     |      Boost.Python.enum
     |      builtins.int
     |      builtins.object
     |  
     |  Data and other attributes defined here:
     |  
     |  Annotations = WasmEdge.Proposal.Annotations
     |  
     |  BulkMemoryOperations = WasmEdge.Proposal.BulkMemoryOperations
     |  
     |  ExceptionHandling = WasmEdge.Proposal.ExceptionHandling
     |  
     |  FunctionReferences = WasmEdge.Proposal.FunctionReferences
     |  
     |  Memory64 = WasmEdge.Proposal.Memory64
     |  
     |  NonTrapFloatToIntConversions = WasmEdge.Proposal.NonTrapFloatToIntConv...
     |  
     |  ReferenceTypes = WasmEdge.Proposal.ReferenceTypes
     |  
     |  SIMD = WasmEdge.Proposal.SIMD
     |  
     |  TailCall = WasmEdge.Proposal.TailCall
     |  
     |  Threads = WasmEdge.Proposal.Threads
     |  
     |  names = {'Annotations': WasmEdge.Proposal.Annotations, 'BulkMemoryOper...
     |  
     |  values = {0: WasmEdge.Proposal.BulkMemoryOperations, 1: WasmEdge.Propo...
     |  
     |  ----------------------------------------------------------------------
     |  Methods inherited from Boost.Python.enum:
     |  
     |  __repr__(self, /)
     |      Return repr(self).
     |  
     |  __str__(self, /)
     |      Return str(self).
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from Boost.Python.enum:
     |  
     |  name
     |  
     |  ----------------------------------------------------------------------
     |  Methods inherited from builtins.int:
     |  
     |  __abs__(self, /)
     |      abs(self)
     |  
     |  __add__(self, value, /)
     |      Return self+value.
     |  
     |  __and__(self, value, /)
     |      Return self&value.
     |  
     |  __bool__(self, /)
     |      self != 0
     |  
     |  __ceil__(...)
     |      Ceiling of an Integral returns itself.
     |  
     |  __divmod__(self, value, /)
     |      Return divmod(self, value).
     |  
     |  __eq__(self, value, /)
     |      Return self==value.
     |  
     |  __float__(self, /)
     |      float(self)
     |  
     |  __floor__(...)
     |      Flooring an Integral returns itself.
     |  
     |  __floordiv__(self, value, /)
     |      Return self//value.
     |  
     |  __format__(self, format_spec, /)
     |      Default object formatter.
     |  
     |  __ge__(self, value, /)
     |      Return self>=value.
     |  
     |  __getattribute__(self, name, /)
     |      Return getattr(self, name).
     |  
     |  __getnewargs__(self, /)
     |  
     |  __gt__(self, value, /)
     |      Return self>value.
     |  
     |  __hash__(self, /)
     |      Return hash(self).
     |  
     |  __index__(self, /)
     |      Return self converted to an integer, if self is suitable for use as an index into a list.
     |  
     |  __int__(self, /)
     |      int(self)
     |  
     |  __invert__(self, /)
     |      ~self
     |  
     |  __le__(self, value, /)
     |      Return self<=value.
     |  
     |  __lshift__(self, value, /)
     |      Return self<<value.
     |  
     |  __lt__(self, value, /)
     |      Return self<value.
     |  
     |  __mod__(self, value, /)
     |      Return self%value.
     |  
     |  __mul__(self, value, /)
     |      Return self*value.
     |  
     |  __ne__(self, value, /)
     |      Return self!=value.
     |  
     |  __neg__(self, /)
     |      -self
     |  
     |  __or__(self, value, /)
     |      Return self|value.
     |  
     |  __pos__(self, /)
     |      +self
     |  
     |  __pow__(self, value, mod=None, /)
     |      Return pow(self, value, mod).
     |  
     |  __radd__(self, value, /)
     |      Return value+self.
     |  
     |  __rand__(self, value, /)
     |      Return value&self.
     |  
     |  __rdivmod__(self, value, /)
     |      Return divmod(value, self).
     |  
     |  __rfloordiv__(self, value, /)
     |      Return value//self.
     |  
     |  __rlshift__(self, value, /)
     |      Return value<<self.
     |  
     |  __rmod__(self, value, /)
     |      Return value%self.
     |  
     |  __rmul__(self, value, /)
     |      Return value*self.
     |  
     |  __ror__(self, value, /)
     |      Return value|self.
     |  
     |  __round__(...)
     |      Rounding an Integral returns itself.
     |      Rounding with an ndigits argument also returns an integer.
     |  
     |  __rpow__(self, value, mod=None, /)
     |      Return pow(value, self, mod).
     |  
     |  __rrshift__(self, value, /)
     |      Return value>>self.
     |  
     |  __rshift__(self, value, /)
     |      Return self>>value.
     |  
     |  __rsub__(self, value, /)
     |      Return value-self.
     |  
     |  __rtruediv__(self, value, /)
     |      Return value/self.
     |  
     |  __rxor__(self, value, /)
     |      Return value^self.
     |  
     |  __sizeof__(self, /)
     |      Returns size in memory, in bytes.
     |  
     |  __sub__(self, value, /)
     |      Return self-value.
     |  
     |  __truediv__(self, value, /)
     |      Return self/value.
     |  
     |  __trunc__(...)
     |      Truncating an Integral returns itself.
     |  
     |  __xor__(self, value, /)
     |      Return self^value.
     |  
     |  as_integer_ratio(self, /)
     |      Return integer ratio.
     |      
     |      Return a pair of integers, whose ratio is exactly equal to the original int
     |      and with a positive denominator.
     |      
     |      >>> (10).as_integer_ratio()
     |      (10, 1)
     |      >>> (-10).as_integer_ratio()
     |      (-10, 1)
     |      >>> (0).as_integer_ratio()
     |      (0, 1)
     |  
     |  bit_length(self, /)
     |      Number of bits necessary to represent self in binary.
     |      
     |      >>> bin(37)
     |      '0b100101'
     |      >>> (37).bit_length()
     |      6
     |  
     |  conjugate(...)
     |      Returns self, the complex conjugate of any int.
     |  
     |  to_bytes(self, /, length, byteorder, *, signed=False)
     |      Return an array of bytes representing an integer.
     |      
     |      length
     |        Length of bytes object to use.  An OverflowError is raised if the
     |        integer is not representable with the given number of bytes.
     |      byteorder
     |        The byte order used to represent the integer.  If byteorder is 'big',
     |        the most significant byte is at the beginning of the byte array.  If
     |        byteorder is 'little', the most significant byte is at the end of the
     |        byte array.  To request the native byte order of the host system, use
     |        `sys.byteorder' as the byte order value.
     |      signed
     |        Determines whether two's complement is used to represent the integer.
     |        If signed is False and a negative integer is given, an OverflowError
     |        is raised.
     |  
     |  ----------------------------------------------------------------------
     |  Class methods inherited from builtins.int:
     |  
     |  from_bytes(bytes, byteorder, *, signed=False) from builtins.type
     |      Return the integer represented by the given array of bytes.
     |      
     |      bytes
     |        Holds the array of bytes to convert.  The argument must either
     |        support the buffer protocol or be an iterable object producing bytes.
     |        Bytes and bytearray are examples of built-in objects that support the
     |        buffer protocol.
     |      byteorder
     |        The byte order used to represent the integer.  If byteorder is 'big',
     |        the most significant byte is at the beginning of the byte array.  If
     |        byteorder is 'little', the most significant byte is at the end of the
     |        byte array.  To request the native byte order of the host system, use
     |        `sys.byteorder' as the byte order value.
     |      signed
     |        Indicates whether two's complement is used to represent the integer.
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from builtins.int:
     |  
     |  __new__(*args, **kwargs) from builtins.type
     |      Create and return a new object.  See help(type) for accurate signature.
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from builtins.int:
     |  
     |  denominator
     |      the denominator of a rational number in lowest terms
     |  
     |  imag
     |      the imaginary part of a complex number
     |  
     |  numerator
     |      the numerator of a rational number in lowest terms
     |  
     |  real
     |      the real part of a complex number
    
    class Result(Boost.Python.instance)
     |  Method resolution order:
     |      Result
     |      Boost.Python.instance
     |      builtins.object
     |  
     |  Static methods defined here:
     |  
     |  __bool__(...)
     |      __bool__( (Result)arg1) -> bool :
     |      
     |          C++ signature :
     |              bool __bool__(pysdk::result {lvalue})
     |  
     |  __init__(...)
     |      __init__( (object)arg1) -> None :
     |      
     |          C++ signature :
     |              void __init__(_object*)
     |  
     |  __reduce__ = <unnamed Boost.Python function>(...)
     |  
     |  __str__(...)
     |      __str__( (Result)arg1) -> str :
     |      
     |          C++ signature :
     |              char const* __str__(pysdk::result {lvalue})
     |  
     |  code(...)
     |      code( (Result)arg1) -> int :
     |      
     |          C++ signature :
     |              int code(pysdk::result {lvalue})
     |  
     |  message(...)
     |      message( (Result)arg1) -> str :
     |      
     |          C++ signature :
     |              char const* message(pysdk::result {lvalue})
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  __instance_size__ = 24
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from Boost.Python.instance:
     |  
     |  __new__(*args, **kwargs) from Boost.Python.class
     |      Create and return a new object.  See help(type) for accurate signature.
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from Boost.Python.instance:
     |  
     |  __dict__
     |  
     |  __weakref__
    
    class Store(Boost.Python.instance)
     |  Method resolution order:
     |      Store
     |      Boost.Python.instance
     |      builtins.object
     |  
     |  Static methods defined here:
     |  
     |  __init__(...)
     |      __init__( (object)arg1) -> None :
     |      
     |          C++ signature :
     |              void __init__(_object*)
     |  
     |  __reduce__ = <unnamed Boost.Python function>(...)
     |  
     |  listFunctions(...)
     |      listFunctions( (Store)arg1, (int)arg2) -> list :
     |      
     |          C++ signature :
     |              boost::python::list listFunctions(pysdk::Store {lvalue},int)
     |  
     |  listModules(...)
     |      listModules( (Store)arg1, (int)arg2) -> list :
     |      
     |          C++ signature :
     |              boost::python::list listModules(pysdk::Store {lvalue},int)
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  __instance_size__ = 24
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from Boost.Python.instance:
     |  
     |  __new__(*args, **kwargs) from Boost.Python.class
     |      Create and return a new object.  See help(type) for accurate signature.
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from Boost.Python.instance:
     |  
     |  __dict__
     |  
     |  __weakref__
    
    class VM(Boost.Python.instance)
     |  Method resolution order:
     |      VM
     |      Boost.Python.instance
     |      builtins.object
     |  
     |  Static methods defined here:
     |  
     |  __init__(...)
     |      __init__( (object)arg1) -> None :
     |      
     |          C++ signature :
     |              void __init__(_object*)
     |      
     |      __init__( (object)arg1, (Configure)arg2) -> None :
     |      
     |          C++ signature :
     |              void __init__(_object*,pysdk::Configure {lvalue})
     |      
     |      __init__( (object)arg1, (Store)arg2) -> None :
     |      
     |          C++ signature :
     |              void __init__(_object*,pysdk::Store {lvalue})
     |      
     |      __init__( (object)arg1, (Configure)arg2, (Store)arg3) -> None :
     |      
     |          C++ signature :
     |              void __init__(_object*,pysdk::Configure {lvalue},pysdk::Store {lvalue})
     |  
     |  __reduce__ = <unnamed Boost.Python function>(...)
     |  
     |  run(...)
     |      run( (VM)arg1, (object)arg2, (object)arg3, (object)arg4, (object)arg5) -> tuple :
     |      
     |          C++ signature :
     |              boost::python::tuple run(pysdk::VM {lvalue},boost::python::api::object,boost::python::api::object,boost::python::api::object,boost::python::api::object)
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  __instance_size__ = 24
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from Boost.Python.instance:
     |  
     |  __new__(*args, **kwargs) from Boost.Python.class
     |      Create and return a new object.  See help(type) for accurate signature.
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors inherited from Boost.Python.instance:
     |  
     |  __dict__
     |  
     |  __weakref__

FUNCTIONS
    version(...)
        version() -> str :
        
            C++ signature :
                char const* version()

DATA
    Annotations = WasmEdge.Proposal.Annotations
    BulkMemoryOperations = WasmEdge.Proposal.BulkMemoryOperations
    ExceptionHandling = WasmEdge.Proposal.ExceptionHandling
    FunctionReferences = WasmEdge.Proposal.FunctionReferences
    Memory64 = WasmEdge.Proposal.Memory64
    NonTrapFloatToIntConversions = WasmEdge.Proposal.NonTrapFloatToIntConv...
    ReferenceTypes = WasmEdge.Proposal.ReferenceTypes
    SIMD = WasmEdge.Proposal.SIMD
    TailCall = WasmEdge.Proposal.TailCall
    Threads = WasmEdge.Proposal.Threads
    Wasi = WasmEdge.Host.Wasi
    WasmEdge = WasmEdge.Host.WasmEdge

FILE
    /home/satacker/wasm_work/WasmEdge/bindings/python/WasmEdge.so


```
