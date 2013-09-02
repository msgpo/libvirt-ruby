/*
 * network.c: virNetwork methods
 *
 * Copyright (C) 2007,2010 Red Hat Inc.
 * Copyright (C) 2013 Chris Lalancette <clalancette@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <ruby.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include "common.h"
#include "connect.h"
#include "extconf.h"

#if HAVE_TYPE_VIRNETWORKPTR
static VALUE c_network;

static void network_free(void *d)
{
    generic_free(Network, d);
}

static virNetworkPtr network_get(VALUE n)
{
    generic_get(Network, n);
}

VALUE network_new(virNetworkPtr n, VALUE conn)
{
    return generic_new(c_network, n, conn, network_free);
}

/*
 * call-seq:
 *   net.undefine -> nil
 *
 * Call virNetworkUndefine[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkUndefine]
 * to undefine this network.
 */
static VALUE libvirt_netw_undefine(VALUE n)
{
    gen_call_void(virNetworkUndefine, connect_get(n), network_get(n));
}

/*
 * call-seq:
 *   net.create -> nil
 *
 * Call virNetworkCreate[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkCreate]
 * to start this network.
 */
static VALUE libvirt_netw_create(VALUE n)
{
    gen_call_void(virNetworkCreate, connect_get(n), network_get(n));
}

/*
 * call-seq:
 *   net.update -> nil
 *
 * Call virNetworkUpdate[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkUpdate]
 * to update this network.
 */
static VALUE libvirt_netw_update(VALUE n, VALUE command, VALUE section,
                                 VALUE index, VALUE xml, VALUE flags)
{
    gen_call_void(virNetworkUpdate, connect_get(n), network_get(n),
         NUM2UINT(command), NUM2UINT(section), NUM2INT(index),
         StringValuePtr(xml), NUM2UINT(flags));
}
/*
 * call-seq:
 *   net.destroy -> nil
 *
 * Call virNetworkDestroy[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkDestroy]
 * to shutdown this network.
 */
static VALUE libvirt_netw_destroy(VALUE n)
{
    gen_call_void(virNetworkDestroy, connect_get(n), network_get(n));
}

/*
 * call-seq:
 *   net.name -> string
 *
 * Call virNetworkGetName[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkGetName]
 * to retrieve the name of this network.
 */
static VALUE libvirt_netw_name(VALUE n)
{
    gen_call_string(virNetworkGetName, connect_get(n), 0, network_get(n));
}

/*
 * call-seq:
 *   net.uuid -> string
 *
 * Call virNetworkGetUUIDString[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkGetUUIDString]
 * to retrieve the UUID of this network.
 */
static VALUE libvirt_netw_uuid(VALUE n)
{
    virNetworkPtr netw = network_get(n);
    char uuid[VIR_UUID_STRING_BUFLEN];
    int r;

    r = virNetworkGetUUIDString(netw, uuid);
    _E(r < 0, create_error(e_RetrieveError, "virNetworkGetUUIDString",
                           connect_get(n)));

    return rb_str_new2((char *) uuid);
}

/*
 * call-seq:
 *   net.xml_desc(flags=0) -> string
 *
 * Call virNetworkGetXMLDesc[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkGetXMLDesc]
 * to retrieve the XML for this network.
 */
static VALUE libvirt_netw_xml_desc(int argc, VALUE *argv, VALUE n)
{
    VALUE flags;

    rb_scan_args(argc, argv, "01", &flags);

    flags = integer_default_if_nil(flags, 0);

    gen_call_string(virNetworkGetXMLDesc, connect_get(n), 1, network_get(n),
                    NUM2UINT(flags));
}

/*
 * call-seq:
 *   net.bridge_name -> string
 *
 * Call virNetworkGetBridgeName[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkGetBridgeName]
 * to retrieve the bridge name for this network.
 */
static VALUE libvirt_netw_bridge_name(VALUE n)
{
    gen_call_string(virNetworkGetBridgeName, connect_get(n), 1, network_get(n));
}

/*
 * call-seq:
 *   net.autostart? -> [true|false]
 *
 * Call virNetworkGetAutostart[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkGetAutostart]
 * to determine if this network will be autostarted when libvirtd starts.
 */
static VALUE libvirt_netw_autostart(VALUE n)
{
    virNetworkPtr netw = network_get(n);
    int r, autostart;

    r = virNetworkGetAutostart(netw, &autostart);
    _E(r < 0, create_error(e_RetrieveError, "virNetworkAutostart",
                           connect_get(n)));

    return autostart ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   net.autostart = [true|false]
 *
 * Call virNetworkSetAutostart[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkSetAutostart]
 * to set this network to be autostarted when libvirtd starts.
 */
static VALUE libvirt_netw_autostart_set(VALUE n, VALUE autostart)
{
    if (autostart != Qtrue && autostart != Qfalse) {
        rb_raise(rb_eTypeError,
                 "wrong argument type (expected TrueClass or FalseClass)");
    }

    gen_call_void(virNetworkSetAutostart, connect_get(n), network_get(n),
                  RTEST(autostart) ? 1 : 0);
}

/*
 * call-seq:
 *   net.free -> nil
 *
 * Call virNetworkFree[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkFree]
 * to free this network.  The object will no longer be valid after this call.
 */
static VALUE libvirt_netw_free(VALUE n)
{
    gen_call_free(Network, n);
}

#if HAVE_VIRNETWORKISACTIVE
/*
 * call-seq:
 *   net.active? -> [true|false]
 *
 * Call virNetworkIsActive[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkIsActive]
 * to determine if this network is currently active.
 */
static VALUE libvirt_netw_active_p(VALUE n)
{
    gen_call_truefalse(virNetworkIsActive, connect_get(n), network_get(n));
}
#endif

#if HAVE_VIRNETWORKISPERSISTENT
/*
 * call-seq:
 *   net.persistent? -> [true|false]
 *
 * Call virNetworkIsPersistent[http://www.libvirt.org/html/libvirt-libvirt.html#virNetworkIsPersistent]
 * to determine if this network is persistent.
 */
static VALUE libvirt_netw_persistent_p(VALUE n)
{
    gen_call_truefalse(virNetworkIsPersistent, connect_get(n), network_get(n));
}
#endif

#endif

/*
 * Class Libvirt::Network
 */
void init_network()
{
#if HAVE_TYPE_VIRNETWORKPTR
    c_network = rb_define_class_under(m_libvirt, "Network", rb_cObject);
    rb_define_attr(c_network, "connection", 1, 0);

    rb_define_method(c_network, "undefine", libvirt_netw_undefine, 0);
    rb_define_method(c_network, "create", libvirt_netw_create, 0);
    rb_define_method(c_network, "update", libvirt_netw_update, 5);
    rb_define_method(c_network, "destroy", libvirt_netw_destroy, 0);
    rb_define_method(c_network, "name", libvirt_netw_name, 0);
    rb_define_method(c_network, "uuid", libvirt_netw_uuid, 0);
    rb_define_method(c_network, "xml_desc", libvirt_netw_xml_desc, -1);
    rb_define_method(c_network, "bridge_name", libvirt_netw_bridge_name, 0);
    rb_define_method(c_network, "autostart", libvirt_netw_autostart, 0);
    rb_define_method(c_network, "autostart?", libvirt_netw_autostart, 0);
    rb_define_method(c_network, "autostart=", libvirt_netw_autostart_set, 1);
    rb_define_method(c_network, "free", libvirt_netw_free, 0);
#if HAVE_VIRNETWORKISACTIVE
    rb_define_method(c_network, "active?", libvirt_netw_active_p, 0);
#endif
#if HAVE_VIRNETWORKISPERSISTENT
    rb_define_method(c_network, "persistent?", libvirt_netw_persistent_p, 0);
#endif
#if HAVE_CONST_VIR_NETWORK_UPDATE_COMMAND_NONE
    rb_define_const(c_network, "NETWORK_UPDATE_COMMAND_NONE", INT2NUM(VIR_NETWORK_UPDATE_COMMAND_NONE));
    rb_define_const(c_network, "NETWORK_UPDATE_COMMAND_MODIFY", INT2NUM(VIR_NETWORK_UPDATE_COMMAND_MODIFY));
    rb_define_const(c_network, "NETWORK_UPDATE_COMMAND_ADD_LAST", INT2NUM(VIR_NETWORK_UPDATE_COMMAND_ADD_LAST));
    rb_define_const(c_network, "NETWORK_UPDATE_COMMAND_ADD_FIRST", INT2NUM(VIR_NETWORK_UPDATE_COMMAND_ADD_FIRST));
    rb_define_const(c_network, "NETWORK_SECTION_NONE", INT2NUM(VIR_NETWORK_SECTION_NONE));
    rb_define_const(c_network, "NETWORK_SECTION_BRIDGE", INT2NUM(VIR_NETWORK_SECTION_BRIDGE));
    rb_define_const(c_network, "NETWORK_SECTION_DOMAIN", INT2NUM(VIR_NETWORK_SECTION_DOMAIN));
    rb_define_const(c_network, "NETWORK_SECTION_IP", INT2NUM(VIR_NETWORK_SECTION_IP));
    rb_define_const(c_network, "NETWORK_SECTION_IP_DHCP_HOST", INT2NUM(VIR_NETWORK_SECTION_IP_DHCP_HOST));
    rb_define_const(c_network, "NETWORK_SECTION_IP_DHCP_RANGE", INT2NUM(VIR_NETWORK_SECTION_IP_DHCP_RANGE));
    rb_define_const(c_network, "NETWORK_SECTION_FORWARD", INT2NUM(VIR_NETWORK_SECTION_FORWARD));
    rb_define_const(c_network, "NETWORK_SECTION_FORWARD_INTERFACE", INT2NUM(VIR_NETWORK_SECTION_FORWARD_INTERFACE));
    rb_define_const(c_network, "NETWORK_SECTION_FORWARD_PF", INT2NUM(VIR_NETWORK_SECTION_FORWARD_PF));
    rb_define_const(c_network, "NETWORK_SECTION_PORTGROUP", INT2NUM(VIR_NETWORK_SECTION_PORTGROUP));
    rb_define_const(c_network, "NETWORK_SECTION_DNS_HOST", INT2NUM(VIR_NETWORK_SECTION_DNS_HOST));
    rb_define_const(c_network, "NETWORK_SECTION_DNS_TXT", INT2NUM(VIR_NETWORK_SECTION_DNS_TXT));
    rb_define_const(c_network, "NETWORK_SECTION_DNS_SRV", INT2NUM(VIR_NETWORK_SECTION_DNS_SRV));
    rb_define_const(c_network, "NETWORK_UPDATE_AFFECT_CURRENT", INT2NUM(VIR_NETWORK_UPDATE_AFFECT_CURRENT));
    rb_define_const(c_network, "NETWORK_UPDATE_AFFECT_LIVE", INT2NUM(VIR_NETWORK_UPDATE_AFFECT_LIVE));
    rb_define_const(c_network, "NETWORK_UPDATE_AFFECT_CONFIG", INT2NUM(VIR_NETWORK_UPDATE_AFFECT_CONFIG));
#endif
#endif
}
