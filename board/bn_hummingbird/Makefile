#
# Based on omap4430sdp
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= lib$(BOARD).a

OBJS	:= bn_hummingbird.o mmc.o bootmode.o boot.o lowbatt_charge.o

CPPFLAGS += BN_HUMMINGBIRD

$(LIB):	$(OBJS)
	$(AR) crv $@ $^

boot.o:
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm $(patsubst %.o,%.rle,$@) $@
lowbatt_charge.o:
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm $(patsubst %.o,%.rle,$@) $@

clean:
	rm -f $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

.depend:	Makefile $(OBJS:.o=.c)
		$(CC) -M $(CPPFLAGS) $(OBJS:.o=.c) > $@

-include .depend

#########################################################################
