/* vim: set expandtab ts=4 sw=4: */
/*
 * You may redistribute this program and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define string_strcmp
#define string_strlen
#include "admin/testframework/AdminTestFramework.h"
#include "admin/Admin.h"
#include "admin/AdminClient.h"
#include "benc/Dict.h"
#include "benc/String.h"
#include "benc/Int.h"
#include "interface/UDPInterface_pvt.h"
#include "memory/Allocator.h"
#include "memory/MallocAllocator.h"
#include "memory/CanaryAllocator.h"
#include "interface/InterfaceController.h"
#include "io/FileWriter.h"
#include "io/Writer.h"
#include "util/Assert.h"
#include "util/log/Log.h"
#include "util/log/WriterLog.h"
#include "util/platform/libc/string.h"
#include "util/Timeout.h"

#ifdef BSD
    #include <netinet/in.h>
#endif

#include <event2/event.h>

/*
 * Setup 2 UDPInterface's, test sending traffic between them.
 */

static int registerPeer(struct InterfaceController* ic,
                        uint8_t herPublicKey[32],
                        String* password,
                        bool requireAuth,
                        struct Interface* iface)
{
    return 0;
}

int receiveMessageACount = 0;
static uint8_t receiveMessageA(struct Message* msg, struct Interface* iface)
{
    // pong
    receiveMessageACount++;
    return iface->sendMessage(msg, iface);
}

static uint8_t receiveMessageB(struct Message* msg, struct Interface* iface)
{
    if (receiveMessageACount) {
        // Got the message, test successful.
        exit(0);
    }
    return 0;
}

static void fail(void* ignored)
{
    Assert_true(!"timeout");
}

int main(int argc, char** argv)
{
    struct Allocator* alloc = CanaryAllocator_new(MallocAllocator_new(1<<20), NULL);
    struct EventBase* base = EventBase_new(alloc);
    struct Writer* logWriter = FileWriter_new(stdout, alloc);
    struct Log* logger = WriterLog_new(logWriter, alloc);

    // mock interface controller.
    struct InterfaceController ic = {
        .registerPeer = registerPeer
    };

    struct UDPInterface* udpA = UDPInterface_new(base, "127.0.0.1", alloc, NULL, logger, &ic);
    struct UDPInterface* udpB = UDPInterface_new(base, "127.0.0.1", alloc, NULL, logger, &ic);


    struct sockaddr_in sin = { .sin_family = AF_INET };
    sin.sin_port = udpA->boundPort_be;
    uint8_t localHost[] = {127, 0, 0, 1};
    Bits_memcpyConst(&sin.sin_addr, localHost, 4);

    struct Message* msg;
    Message_STACK(msg, 0, 128);

    Message_push(msg, "Hello World", 12);
    Message_push(msg, &sin, sizeof(struct sockaddr_in));

    udpA->generic.receiveMessage = receiveMessageA;
    udpB->generic.receiveMessage = receiveMessageB;

    udpB->generic.sendMessage(msg, &udpB->generic);

    Timeout_setTimeout(fail, NULL, 1000, base, alloc);

    EventBase_beginLoop(base);
}
