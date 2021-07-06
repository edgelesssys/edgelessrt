// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

int fcntl64(int fd, int cmd, void* arg)
{
    int fcntl();
    return fcntl(fd, cmd, arg);
}
