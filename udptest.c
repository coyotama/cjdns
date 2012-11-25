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
#include "crypto/Random.h"
#include "exception/Except.h"
#include "memory/Allocator.h"
#include "memory/BufferAllocator.h"
#include "util/Assert.h"
#include "util/Errno.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <event2/event.h>

int main(int argc, char** argv)
{
    if (argc < 3) {
        printf("Usage: %s 4.5.6.7:8901 1.2.3.4:5678\n", argv[0]);
        printf("send a udp packet to ip address 1.2.3.4 port 5678\n");
        printf("bind to address 4.5.6.7:8901 and send from this address.\n");
        return 0;
    }


    struct sockaddr_storage localaddr;
    int localaddrLen = sizeof(struct sockaddr_storage);
    if (0 != evutil_parse_sockaddr_port(argv[1], (struct sockaddr*) &localaddr, &localaddrLen)) {
        Except_raise(NULL, -1, "failed to parse local address");
    }

    struct sockaddr_storage addr;
    int addrLen = sizeof(struct sockaddr_storage);
    if (0 != evutil_parse_sockaddr_port(argv[2], (struct sockaddr*) &addr, &addrLen)) {
        Except_raise(NULL, -1, "failed to parse remote address");
    }

    int sock = socket(localaddr.ss_family, SOCK_DGRAM, 0);
    if (sock == -1) {
        Except_raise(NULL, -1, "call to socket() failed.");
    }

    if (bind(sock, (struct sockaddr*) &localaddr, localaddrLen)) {
        Except_raise(NULL, -1, "call to bind() failed.");
    }

    evutil_make_socket_nonblocking(sock);

    uint8_t buff[256];
    struct Allocator* alloc;
    BufferAllocator_STACK(alloc, 256);
    struct Random* rand = Random_new(alloc, NULL);
    Random_bytes(rand, buff, 256);

    if (sendto(sock, buff, 256, 0, (struct sockaddr*) &addr, addrLen) < 0) {
        printf("Error in sendto() [%s]", Errno_getString());
    }
}
