#ifndef PYWASMEDGE_UTILS_HPP
#define PYWASMEDGE_UTILS_HPP

#include <boost/python.hpp>

template <typename T> boost::python::object transfer_to_python(T *t) {
  std::unique_ptr<T> ptr(t);
  typename boost::python::manage_new_object::apply<T *>::type converter;
  boost::python::handle<> handle(converter(*ptr));
  ptr.release();

  return boost::python::object(handle);
}

template <typename T> boost::python::object transfer_to_python(T t) {
  return boost::python::object(t);
}

#endif // PYWASMEDGE_UTILS_HPP