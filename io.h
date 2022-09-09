/*
 * Copyright (c) 2022 A.W. van Halderen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IO_H
#define IO_H

static inline int
readfully(int fd, char* buffer, int size)
{
    int count;
    int index = 0;
    while(index < size) {
        count = read(fd, &(buffer[index]), size - index);
        if(count <= 0)
            return (count < 0 ? -1 : 1);
        index += count;
    }
    return 0;
}

static inline int
writefully(int fd, char* buffer, int size)
{
    int count;
    int index = 0;
    while(index < size) {
        count = write(fd, &(buffer[index]), size - index);
        if(count < 0 && errno != EAGAIN && errno != EINTR)
            return -1;
        else
            index += count;
    }
    return 0;
}

static inline int
readpartially(int fd, char* buffer, int* index, int size)
{
    int count;
    count = read(fd, &(buffer[*index]), size - *index);
    if(count <= 0) {
        *index = 0;
        return 0;
    }
    *index += count;
    if(*index >= size)
        return 1;
    else
        return 0;
}

#endif
