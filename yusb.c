/*
 * yusb.c --
 *
 * Implements Yorick interface to libusb (http://libusbx.org/).
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (c) 2014 Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>.
 *
 * The MIT License (MIT):
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * -----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pstdlib.h>
#include <yapi.h>

#include <libusb.h>

#define TRUE  1
#define FALSE 0

/* Define some macros to get rid of some GNU extensions when not compiling
   with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#   define __attribute__(x)
#   define __inline__
#   define __FUNCTION__        ""
#   define __PRETTY_FUNCTION__ ""
#endif

PLUG_API void y_error(const char *) __attribute__ ((noreturn));
static void push_string(const char* str);

/* For now there is a single libusb context for a given Yorick session. */
static libusb_context* ctx = NULL;
static libusb_device** dev_list = NULL;
static ssize_t dev_count = 0;

static void initialize();
#define INITIALIZE if (ctx != NULL) ; else initialize()

#define _JOIN(a,b) a ## b
#define JOIN2(a,b) _JOIN(a,b)

#define ERROR_TABLE \
  ERROR_ENTRY(SUCCESS, "Success (no error)") \
  ERROR_ENTRY(ERROR_IO, "Input/output error.") \
  ERROR_ENTRY(ERROR_INVALID_PARAM, "Invalid parameter.") \
  ERROR_ENTRY(ERROR_ACCESS, "Access denied (insufficient permissions)") \
  ERROR_ENTRY(ERROR_NO_DEVICE, "No such device (it may have been disconnected)") \
  ERROR_ENTRY(ERROR_NOT_FOUND, "Entity not found.") \
  ERROR_ENTRY(ERROR_BUSY, "Resource busy.") \
  ERROR_ENTRY(ERROR_TIMEOUT, "Operation timed out.") \
  ERROR_ENTRY(ERROR_OVERFLOW, "Overflow.") \
  ERROR_ENTRY(ERROR_PIPE, "Pipe error.") \
  ERROR_ENTRY(ERROR_INTERRUPTED, "System call interrupted (perhaps due to signal)") \
  ERROR_ENTRY(ERROR_NO_MEM, "Insufficient memory.") \
  ERROR_ENTRY(ERROR_NOT_SUPPORTED, "Operation not supported or unimplemented on this platform.") \
  ERROR_ENTRY(ERROR_OTHER, "Other error.")

static const char* get_error_name(int code)
{
#undef ERROR_ENTRY
#define ERROR_ENTRY(a,b) case LIBUSB_##a: return "LIBUSB_" #a;
  switch(code) {
    ERROR_TABLE
  default:
    return "UNKNOWN";
  }
}

static const char* get_error_description(int code)
{
#undef ERROR_ENTRY
#define ERROR_ENTRY(a,b) case LIBUSB_##a: return b;
  switch(code) {
    ERROR_TABLE
  default:
    return "Unknown error.";
  }
}

void Y_usb_error_name(int argc)
{
  if (argc != 1) y_error("expected exactly one argument");
  push_string(get_error_name(ygets_i(0)));
}

void Y_usb_error_description(int argc)
{
  if (argc != 1) y_error("expected exactly one argument");
  push_string(get_error_description(ygets_i(0)));
}

static void failure(const char* reason, int code)
{
  static char msg[256];
  if (reason == NULL || reason[0] == 0) {
    y_error(get_error_description(code));
  } else {
    sprintf(msg, "%s [%s]",  reason, get_error_name(code));
    y_error(msg);
  }
}

static void push_string(const char* str)
{
  ypush_q(NULL)[0] = p_strcpy(str);
}

static void free_dev_list(void)
{
  if (dev_count > 0) {
    dev_count = 0;
    libusb_free_device_list(dev_list, 1);
  }
}

static void load_device_list(void)
{
  INITIALIZE;
  free_dev_list(); /* in case of interrupts */
  dev_count = libusb_get_device_list(ctx, &dev_list);
  if (dev_count < 0) {
    y_error("failed to get USB devices list");
  }
}

/* must only be called on exit, so interrupts do not matter here */
static void finalize(void)
{
  /* in case of interrupts copy address in a temporary variable */
  libusb_context* tmp = ctx;
  ctx = NULL;
  free_dev_list();
  if (tmp != NULL) {
    libusb_exit(tmp);
  }
}

static void initialize(void)
{
  if (ctx == NULL) {
    int code = libusb_init(&ctx);
    if (code != 0) {
      if (ctx != NULL) {
        y_error("*** ASSERTION FAILED *** non NULL context on libusb_init failure");
      }
      failure(NULL, code);
    }
    if (ctx == NULL) {
      y_error("*** ASSERTION FAILED *** NULL context on libusb_init success");
    }
    ycall_on_quit(finalize);
    libusb_set_debug(ctx, LIBUSB_LOG_LEVEL_NONE);
    libusb_setlocale("en");
  }
}

#define STRING_DESCRIPTOR_SIZE 256

static int
get_string_descriptor(libusb_device_handle* handle, int index,
                      char* string, int length)
{
  return libusb_get_string_descriptor_ascii(handle, (index)&0xff,
                                            (unsigned char*)string,
                                            length);
}

static void define_global_int(const char* name, int value)
{
  ypush_int(value);
  yput_global(yget_global(name, 0), 0);
  yarg_drop(1);
}

void Y__usb_init(int argc)
{
#undef ERROR_ENTRY
#define ERROR_ENTRY(a,b) define_global_int("USB_"#a, LIBUSB_##a);
  ERROR_TABLE;
  ypush_nil();
}

void Y_usb_debug(int argc)
{
  int level;

  if (argc != 1) {
    y_error("expecting exactly one argument");
  }
  level = ygets_i(0);
  INITIALIZE;
  libusb_set_debug(ctx, level);
  ypush_nil();
}

static const char*
get_string(libusb_device_handle* handle, int index)
{
  static char str[STRING_DESCRIPTOR_SIZE];
  int code;
  if (index <= 0) {
    return "unknown";
  }
  code = get_string_descriptor(handle, index, str, sizeof(str));
  if (code != 0) {
    libusb_close(handle);
    failure(NULL, code);
  }
  str[sizeof(str)/sizeof(str[0]) - 1] = 0;
  return str;
}

void Y_usb_summary(int argc)
{
  ssize_t i;
  libusb_device_handle* handle;

  load_device_list();
  for (i = 0; i < dev_count; ++i) {
    int code;
    struct libusb_device_descriptor desc;
    libusb_device* dev = dev_list[i];
    uint8_t bus_number = libusb_get_bus_number(dev);
    uint8_t port_number = libusb_get_port_number(dev);
    uint8_t device_address = libusb_get_device_address(dev);
    fprintf(stdout, "USB Device %ld:\n", (long)i);
    fprintf(stdout, "  Bus Number ---------> %d\n", (int)bus_number);
    fprintf(stdout, "  Port Number --------> %d\n", (int)port_number);
    fprintf(stdout, "  Device Address -----> %d\n", (int)device_address);
    code = libusb_get_device_descriptor(dev, &desc);
    if (code != 0) {
      failure(NULL, code);
    }
    fprintf(stdout, "  Vendor ID ----------> 0x%04x\n",
            (unsigned int)desc.idVendor);
    fprintf(stdout, "  Product ID ---------> 0x%04x\n",
            (unsigned int)desc.idProduct);
    code = libusb_open(dev, &handle);
    if (code == 0) {
      fprintf(stdout, "  Manufacturer -------> %s\n",
              get_string(handle, desc.iManufacturer));
      fprintf(stdout, "  Product ------------> %s\n",
              get_string(handle, desc.idProduct));
      fprintf(stdout, "  Serial Number ------> %s\n",
              get_string(handle, desc.iSerialNumber));
      libusb_close(handle);
    }
  }
  free_dev_list();
  ypush_nil();
}

/*--------------------------------------------------------------------------*/

typedef struct _ydev_instance ydev_instance_t;
struct _ydev_instance {
  libusb_device* device;
  libusb_device_handle* handle;
  struct libusb_device_descriptor descriptor;
  int bus;
  int port;
  int address;
};

static void ydev_free(void *);
static void ydev_print(void *);
/*static void ydev_eval(void *, int);*/
static void ydev_extract(void *, char *);

static y_userobj_t ydev_class = {
  "USB Device",
  ydev_free,
  ydev_print,
  /*ydev_eval*/NULL,
  ydev_extract,
  NULL
};

static void ydev_free(void *self)
{
  ydev_instance_t *obj = (ydev_instance_t *)self;
  if (obj->handle != NULL) {
    libusb_close(obj->handle);
  }
  if (obj->device != NULL) {
    libusb_unref_device(obj->device);
  }
}

static void ydev_print(void *self)
{
  ydev_instance_t *obj = (ydev_instance_t *)self;
  char buf[256];
  y_print(ydev_class.type_name, 0);
  sprintf(buf, ": bus=%d, port=%d, address=%d, vendor=0x%04x, product=0x%04x, manufacturer=0x%04x, serial=0x%04x", obj->bus, obj->port, obj->address, obj->descriptor.idVendor, obj->descriptor.idProduct, obj->descriptor.iManufacturer, obj->descriptor.iSerialNumber);
  y_print(buf, 1);
}

static void ydev_extract(void *addr, char *member)
{
  ydev_instance_t *obj = (ydev_instance_t *)addr;
  int c = (member != NULL ? member[0] : '\0');
  if (c == 'b' && strcmp(member, "bus") == 0) {
    ypush_int(obj->bus);
  } else if (c == 'p' && strcmp(member, "port") == 0) {
    ypush_int(obj->port);
  } else if (c == 'a' && strcmp(member, "address") == 0) {
    ypush_int(obj->address);
  } else if (c == 'v' && strcmp(member, "vendor") == 0) {
    ypush_int(obj->descriptor.idVendor);
  } else if (c == 'p' && strcmp(member, "product") == 0) {
    ypush_int(obj->descriptor.idProduct);
  } else if (c == 'm' && strcmp(member, "manufacturer") == 0) {
    ypush_int(obj->descriptor.iManufacturer);
  } else if (c == 's' && strcmp(member, "serial") == 0) {
    ypush_int(obj->descriptor.iSerialNumber);
  } else {
    y_error("bad member name");
  }
}

/* Get an USB device from the stack. */
static ydev_instance_t* get_device(int iarg)
{
  return (ydev_instance_t*)yget_obj(iarg, &ydev_class);
}

void Y_usb_open_device(int argc)
{
  ydev_instance_t *obj = NULL;
  libusb_device* dev;
  int bus, port;
  int i, ret;

  if (argc != 2) {
    y_error("expecting exactly 2 arguments");
  }
  bus = ygets_i(1);
  port = ygets_i(0);

  load_device_list();
  for (i = 0; i < dev_count; ++i) {
    dev = dev_list[i];
    if (libusb_get_bus_number(dev) == bus &&
        libusb_get_port_number(dev) == port) {
      obj = (ydev_instance_t *)ypush_obj(&ydev_class, sizeof(ydev_instance_t));
      obj->device = libusb_ref_device(dev);
      ret = libusb_open(obj->device, &obj->handle);
      if (ret < 0) {
        obj->handle = NULL;
        failure("failed to open device", ret);
      }
      obj->bus = bus;
      obj->port = port;
      obj->address = libusb_get_device_address(dev);
      ret = libusb_get_device_descriptor(dev, &obj->descriptor);
      if (ret != 0) {
        free_dev_list();
        failure("unable to get device descriptor", ret);
      }
      break;
    }
  }
  free_dev_list();
  if (obj == NULL) {
    ypush_nil();
  }
}

void Y__usb_probe_devices(int argc)
{
  struct libusb_device_descriptor desc;
  libusb_device* dev;
  long dims[3];
  int* data;
  int i, ret;

  if (argc != 1 || ! yarg_nil(0)) {
    y_error("expecting exactly one nil argument");
  }
  load_device_list();
  if (dev_count > 0) {
    dims[0] = 2;
    dims[1] = 7;
    dims[2] = dev_count;
    data = ypush_i(dims);
    for (i = 0; i < dev_count; ++i) {
      dev = dev_list[i];
      ret = libusb_get_device_descriptor(dev, &desc);
      if (ret != 0) {
        free_dev_list();
        failure("unable to get device descriptor", ret);
      }
      data[0] = libusb_get_bus_number(dev);
      data[1] = libusb_get_port_number(dev);
      data[2] = libusb_get_device_address(dev);
      data[3] = desc.idVendor;
      data[4] = desc.idProduct;
      data[5] = desc.iManufacturer;
      data[6] = desc.iSerialNumber;
      data += 7;
    }
  } else {
    ypush_nil();
  }
  free_dev_list();
}

void Y_usb_get_string(int argc)
{
  char str[STRING_DESCRIPTOR_SIZE];
  ydev_instance_t* obj;
  int index;
  int ret;

  if (argc != 2) {
    y_error("expecting exactly 2 arguments");
  }
  obj = get_device(1);
  index = ygets_i(0);

  ret = get_string_descriptor(obj->handle, index, str, sizeof(str));
  if (ret < 0) {
    ypush_int(ret);
  } else {
    str[sizeof(str)/sizeof(str[0]) - 1] = 0;
    push_string(str);
  }
}

void Y_usb_claim_interface(int argc)
{
  ydev_instance_t* obj;
  int interface_number;
  int ret;

  if (argc != 2) {
    y_error("expecting exactly 2 arguments");
  }
  obj = get_device(1);
  interface_number = ygets_i(0);

  ret = libusb_claim_interface(obj->handle, interface_number);
  if (ret < 0 && yarg_subroutine()) {
    failure(NULL, ret);
  }
  ypush_int(ret);
}

void Y_usb_release_interface(int argc)
{
  ydev_instance_t* obj;
  int interface_number;
  int ret;

  if (argc != 2) {
    y_error("expecting exactly 2 arguments");
  }
  obj = get_device(1);
  interface_number = ygets_i(0);


  ret = libusb_release_interface(obj->handle, interface_number);
  if (ret < 0 && yarg_subroutine()) {
    failure(NULL, ret);
  }
  ypush_int(ret);
}

static void* get_data(int iarg, long* the_size)
{
  void* data;
  long size, ntot;
  int type;

  data = NULL;
  size = 0;
  if (! yarg_nil(iarg)) {
    data = ygeta_any(iarg, &ntot, NULL, &type);
    switch(type) {
    case Y_CHAR:
      size = sizeof(char)*ntot;
      break;
    case Y_SHORT:
      size = sizeof(short)*ntot;
      break;
    case Y_INT:
      size = sizeof(int)*ntot;
      break;
    case Y_LONG:
      size = sizeof(long)*ntot;
      break;
    case Y_FLOAT:
      size = sizeof(float)*ntot;
      break;
    case Y_DOUBLE:
      size = sizeof(double)*ntot;
      break;
    case Y_COMPLEX:
      size = 2*sizeof(double)*ntot;
      break;
    default:
      y_error("bad data type");
    }
  }
  if (the_size != NULL) {
    *the_size = size;
  }
  return data;
}

void Y_usb_control_transfer(int argc)
{
  ydev_instance_t* obj;
  int ret;
  int type, request, value, index, length;
  long size;
  unsigned char* data;
  unsigned int timeout;

  /* Get arguments. */
  if (argc != 8) {
    y_error("expecting exactly 8 arguments");
  }
  obj = get_device(7);
  type = ygets_i(6) & 0xff; /* uint8_t */
  request = ygets_i(5) & 0xff; /* uint8_t */
  value = ygets_i(4) & 0xffff; /* uint16_t */
  index = ygets_i(3) & 0xffff; /* uint16_t */
  data = (unsigned char*)get_data(2, &size);
  length = ygets_i(1) & 0xffff; /* uint16_t */
  timeout = (unsigned int)(ygets_l(0) & 0xffffffffL);
  if (length < 0) {
    y_error("invalid length");
  }
  if (length > size) {
    y_error("length must be at most the size of the data");
  }

  /* Apply operation. */
  ret = libusb_control_transfer(obj->handle, type, request, value,
                                index, data, length, timeout);
  if (ret < 0 && yarg_subroutine()) {
    failure(NULL, ret);
  }
  ypush_int(ret);
}

 static void do_transfer(int argc,
                         int (*transfer)(struct libusb_device_handle* handle,
                                         unsigned char endpoint,
                                         unsigned char* data,
                                         int length,
                                         int* transferred,
                                         unsigned int timeout))
{
  ydev_instance_t* obj;
  int ret, iarg;
  int endpoint, length, transferred, offset;
  long size;
  unsigned char* data;
  unsigned int timeout;
  long transferred_index;

  /* Get arguments. */
  if (argc != 6 && argc != 7) {
    y_error("expecting 6 or 7 arguments");
  }
  iarg = argc;
  obj = get_device(--iarg);
  endpoint = ygets_i(--iarg) & 0xff; /* uint8_t */
  data = (unsigned char*)get_data(--iarg, &size);
  length = ygets_i(--iarg);
  if (length < 0) {
    y_error("invalid length");
  }
  if (length > size) {
    y_error("length must be at most the size of the data");
  }
  transferred_index = yget_ref(--iarg);
  if (transferred_index < 0L) {
    y_error("expecting a simple variable reference");
  }
  timeout = (unsigned int)(ygets_l(--iarg) & 0xffffffffL);
  if (iarg > 0) {
    offset = ygets_i(--iarg);
    if (offset < 0 || offset > length) {
      y_error("invalid offset");
    }
  } else {
    offset = 0;
  }

  /* Apply operation. */
  if (length > offset) {
    ret = transfer(obj->handle, endpoint, data + offset,
                   length - offset, &transferred, timeout);
    if (transferred_index >= 0L &&
        (ret == 0 || ret == LIBUSB_ERROR_TIMEOUT)) {
      /* Store total number of transferred bytes so far. */
      ypush_int(transferred + offset);
      yput_global(transferred_index, 0);
    }
    if (ret < 0 && yarg_subroutine()) {
      failure(NULL, ret);
    }
  } else {
    /* Nothing to do. */
    ret = 0;
  }
  ypush_int(ret);
}

void Y_usb_bulk_transfer(int argc)
{
  do_transfer(argc, libusb_bulk_transfer);
}

void Y_usb_interrupt_transfer(int argc)
{
  do_transfer(argc, libusb_interrupt_transfer);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * fill-column: 78
 * coding: utf-8
 * ispell-local-dictionary: "american"
 * End:
 */
