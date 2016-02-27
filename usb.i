/*
 * usb.i --
 *
 * Support for USB in Yorick.
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
 *-----------------------------------------------------------------------------
 */

if (is_func(plug_in)) plug_in, "yusb";

local usb_descriptor;
extern _usb_probe_devices;
func usb_probe_devices(nil)
/* DOCUMENT desc = usb_probe_devices();
     Query the descriptors of current USB devices.  If there are no USB
     devices, the result is nil; otherwise, the result is an array of
     usb_descriptor structures:

       desc(i).bus          - bus number of i-th USB device
       desc(i).port         - port number of i-th USB device
       desc(i).address      - address of i-th USB device
       desc(i).vendor       - vendor identifier of i-th USB device
       desc(i).product      - product identifier of i-th USB device
       desc(i).manufacturer - manufacturer identifier of i-th USB device
       desc(i).serial       - serial number of i-th USB device

     Beware that the returned list is only correct while no USB devices are
     connected nor disconnected.  See usb_open_device for an example of usage.

     When called as a subroutine, a comprehensive list of USB devices is
     printed to standard output, like 'lsusb' Unix command but without the
     string description of each device.  Note that the "Device" number is the
     port number plus one.

  SEE ALSO: usb_open_device.
 */
{
  list = _usb_probe_devices();
  if (is_void(list)) {
    return;
  }
  n = dimsof(list)(3);
  if (am_subroutine()) {
    write, format="Bus %03d Device %03d: ID %04x:%04x\n",
      list(1,), list(2,) + 1, list(4,), list(5,);
  } else {
    out = array(usb_descriptor, n);
    out.bus = list(1,);
    out.port = list(2,);
    out.address = list(3,);
    out.vendor = list(4,);
    out.product = list(5,);
    out.manufacturer = list(6,);
    out.serial = list(7,);
    return out;
  }
}
struct usb_descriptor {
  int bus;
  int port;
  int address;
  int vendor;
  int product;
  int manufacturer;
  int serial;
}

extern usb_open_device;
/* DOCUMENT dev = usb_open_device(bus, port);
     Open the USB device with specified bus and port numbers and return a
     handle for it.  An empty result is returned if no device is currently
     connected to the given bus and port.  Use usb_list_devices to figure out
     a list of connected USB devices.  There is no need to close the device,
     this is automatically done when the device is no longer in use in Yorick.

     The device may be used as a structure object to query some information:
       dev.bus          - bus number of i-th USB device
       dev.port         - port number of i-th USB device
       dev.address      - address of i-th USB device
       dev.vendor       - vendor identifier of i-th USB device
       dev.product      - product identifier of i-th USB device
       dev.manufacturer - manufacturer identifier of i-th USB device
       dev.serial       - serial number of i-th USB device

   EXAMPLE
     // Get available USB devices:
     descr = usb_probe_devices();

     // Select the ones matching vendor and product identifiers:
     sel = where((descr.vendor == SOME_VENDOR)&
                 (descr.product == SOME_PRODUCT));
     if (! is_array(sel)) error, "no USB devices matching vendor/product";

     // Open the first matching one:
     j = sel(1);
     dev = usb_open_device(descr(j).bus, descr(j).port);

     // Check whether changes occured since probing devices:
     if (dev.vendor != SOME_VENDOR || dev.product != SOME_PRODUCT) {
       error, "USB devices changed after probing";
     }


   SEE ALSO: usb_probe_devices.
*/

extern usb_get_string;
/* DOCUMENT str_or_err = usb_get_string(dev, idx);
     Attempt to retrieve the string descriptor corresponding to index IDX for
     USB device DEV.  In case of failure, an error code (that is an integer)
     is returned intead of a string.
*/

extern usb_claim_interface;
extern usb_release_interface;
/* DOCUMENT ret = usb_claim_interface(dev, num);
         or ret = usb_release_interface(dev, num);

     Claim/release interface number NUM for USB device DEV.  Non-zero result
     means failure.  When called as a subroutine, an error is thrown in case
     of failure.
*/

extern usb_control_transfer;
/* DOCUMENT usb_control_transfer(dev, type, request, value,
                                 index, data, length, timeout);

     Perform a USB control transfer.  The direction of the transfer is
     inferred from the request type field of the setup packet.

   ARGUMENTS:
     DEV is a handle for the USB device to communicate with.

     TYPE is the request type field for the setup packet.

     REQUEST is the request field for the setup packet.

     VALUE is the value field for the setup packet.

     INDEX is the index field for the setup packet.

     DATA is a suitably-sized data buffer for either input or output
     (depending on direction bits within TYPE).

     LENGTH is the length field for the setup packet. The DATA buffer should
     be at least this size.

     TIMEOUT is the timeout (in milliseconds) that this function should wait
     before giving up due to no response being received.  For an unlimited
     timeout, use value 0.

   RETURNS:
     On success, the returned value is the number of bytes actually
     transferred.  On failure, a strictly negative value is returned which is
     the error code.  If called as a subroutine, an error is thrown in case of
     failure.

  SEE ALSO: usb_open_device, usb_claim_interface, usb_bulk_transfer,
            usb_interrupt_transfer.
 */

extern usb_bulk_transfer;
/* DOCUMENT usb_bulk_transfer(dev, endpoint, data, length,
                              transferred, timeout);
         or usb_bulk_transfer(dev, endpoint, data, length,
                              transferred, timeout, offset);

     Perform a USB bulk transfer.  The direction of the transfer is inferred
     from the direction bits of the endpoint address.

     For bulk reads, the LENGTH parameter indicates the maximum length of data
     you are expecting to receive.  If less data arrives than expected, this
     function will return that data, so be sure to check the TRANSFERRED
     output parameter.

     You should also check the TRANSFERRED parameter for bulk writes.  Not all
     of the data may have been written.

     Also check TRANSFERRED when dealing with a timeout error code.  libusb
     may have to split your transfer into a number of chunks to satisfy
     underlying O/S requirements, meaning that the timeout may expire after
     the first few chunks have completed.  libusb is careful not to lose any
     data that may have been transferred; do not assume that timeout
     conditions indicate a complete lack of I/O.


   ARGUMENTS:
     DEV is a handle for the device to communicate with.

     ENDPOINT is the address of a valid endpoint to communicate with.

     DATA is a suitably-sized data buffer for either input or output
     (depending on ENDPOINT).

     LENGTH is, for bulk writes, the number of bytes from data to be sent, for
     bulk reads, the maximum number of bytes to receive into the data buffer.

     TRANSFERRED is the variable to store the number of bytes actually
     transferred.

     TIMEOUT is the timeout (in milliseconds) that this function should wait
     before giving up due to no response being received.  For an unlimited
     timeout, use value 0.

     OFFSET is an optional offset in bytes to resume the transfer at that
     position in the DATA buffer.  If specified, then at most LENGTH-OFFSET
     bytes will be transferred but the value stored into the TRANSFERRED
     variable is still the total number of transferred bytes so far.


   RETURNS:
     On success, 0 is returned and the TRANSFERRED variable is set with the
     number of transferred bytes.  On failure, a strictly negative value is
     returned which is the error code; if a timeout occured, then the
     TRANSFERRED variable is set and USB_ERROR_TIMEOUT is returned.  If called
     as a subroutine, an error is thrown in case of failure.

  SEE ALSO: usb_open_device, usb_claim_interface; usb_control_transfer,
            usb_interrupt_transfer.
 */

extern usb_interrupt_transfer;
/* DOCUMENT usb_interrupt_transfer(dev, endpoint, data, length,
                                   transferred, timeout);
         or usb_interrupt_transfer(dev, endpoint, data, length,
                                   transferred, timeout, offset);

     Perform a USB interrupt transfer.  The direction of the transfer is
     inferred from the direction bits of the endpoint address.

     For interrupt reads, the LENGTH parameter indicates the maximum length of
     data you are expecting to receive.  If less data arrives than expected,
     this function will return that data, so be sure to check the TRANSFERRED
     output parameter.

     You should also check the TRANSFERRED parameter for interrupt writes.
     Not all of the data may have been written.

     Also check TRANSFERRED when dealing with a timeout error code.  libusb
     may have to split your transfer into a number of chunks to satisfy
     underlying O/S requirements, meaning that the timeout may expire after
     the first few chunks have completed.  libusb is careful not to lose any
     data that may have been transferred; do not assume that timeout
     conditions indicate a complete lack of I/O.


   ARGUMENTS:
     DEV is a handle for the device to communicate with.  ENDPOINT is the
     address of a valid endpoint to communicate with.

     DATA is a suitably-sized data buffer for either input or output
     (depending on ENDPOINT).

     LENGTH is, for interrupt writes, the number of bytes from data to be
     sent, for interrupt reads, the maximum number of bytes to receive into
     the data buffer.

     TRANSFERRED is the variable to store the number of bytes actually
     transferred.

     TIMEOUT is the timeout (in milliseconds) that this function should wait
     before giving up due to no response being received.  For an unlimited
     timeout, use value 0.

     OFFSET is an optional offset in bytes to resume the transfer at that
     position in the DATA buffer.  If specified, then at most LENGTH-OFFSET
     bytes will be transferred but the value stored into the TRANSFERRED
     variable is still the total number of transferred bytes so far.


   RETURNS:
     On success, 0 is returned and the TRANSFERRED variable is set with the
     number of transferred bytes.  On failure, a strictly negative value is
     returned which is the error code; if a timeout occured, then the
     TRANSFERRED variable is set and USB_ERROR_TIMEOUT is returned.  If called
     as a subroutine, an error is thrown in case of failure.

  SEE ALSO: usb_open_device, usb_claim_interface; usb_control_transfer,
            usb_interrupt_transfer.
 */

local USB_SUCCESS, USB_ERROR_IO, USB_ERROR_INVALID_PARAM, USB_ERROR_ACCESS;
local USB_ERROR_NO_DEVICE, USB_ERROR_NOT_FOUND, USB_ERROR_BUSY;
local USB_ERROR_TIMEOUT, USB_ERROR_OVERFLOW, USB_ERROR_PIPE;
local USB_ERROR_INTERRUPTED, USB_ERROR_NO_MEM, USB_ERROR_NOT_SUPPORTED;
local USB_ERROR_OTHER;
extern usb_error_name;
extern usb_error_description;
/* DOCUMENT str = usb_error_name(code);
         or str = usb_error_description(code);
     Query USB library error name or short description given error code.
*/

func usb_error(msg, ret)
/* DOCUMENT usb_error, msg, ret;
     Throw an error with message "MSG [ERR]" where ERR is the name of the
     USB error code RET.
   SEE ALSO: error, usb_error_name, errs2caller.
 */
{
  error, swrite(format="%s [%s]", msg, usb_error_name(ret));
}
errs2caller, usb_error;

extern usb_debug;
/* DOCUMENT usb_debug, lvl;
     Set log message verbosity for libUSB.  LVL is one of:
       0 : no messages ever printed by the library (LIBUSB_LOG_LEVEL_NONE)
       1 : error messages are printed to stderr (LIBUSB_LOG_LEVEL_ERROR)
       2 : warning and error messages are printed to stderr
          (LIBUSB_LOG_LEVEL_WARNING)
       3 : informational messages are printed to stdout, warning and error
           messages are printed to stderr (LIBUSB_LOG_LEVEL_INFO)
       4 : debug and informational messages are printed to stdout, warnings
           and errors to stderr (LIBUSB_LOG_LEVEL_DEBUG)

     When starting, the default debug level is 0.

   SEE ALSO: usb_probe_devices, usb_open_device.
 */

extern _usb_init;
/* DOCUMENT _usb_init;
     Initialize internals and define constants.  Automatically called,
     so you should not have to call it ever.
 */
_usb_init;
