/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2005 by
 *  	
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- (LS)�			<team@streaming.polito.it>
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <fenice/wsocket.h>

/*return -1 if an error occurs during accept*/
/*return the socket descriptor, note: doesn't shutdown the listened one */
int sock_accept(int sock)
{
	int new_fd;      
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;
	
	memset(&their_addr, 0, sizeof(struct sockaddr_in));

	sin_size = sizeof(struct sockaddr_in);
	new_fd = accept(sock,(struct sockaddr *)&their_addr, &sin_size);

	return new_fd;
}