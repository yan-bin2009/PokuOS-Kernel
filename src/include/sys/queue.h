/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_QUEUE_H_
#define _SYS_QUEUE_H_

#define TAILQ_HEAD(name, type)                                          \
struct name {                                                           \
        struct type *tqh_first;                                         \
        struct type **tqh_last;                                         \
}

#define TAILQ_HEAD_INITIALIZER(head)                                    \
        { NULL, &(head).tqh_first }

#define TAILQ_ENTRY(type)                                               \
struct {                                                                \
        struct type *tqe_next;                                          \
        struct type **tqe_prev;                                         \
}

#define TAILQ_INIT(head) do {                                           \
        (head)->tqh_first = NULL;                                       \
        (head)->tqh_last = &(head)->tqh_first;                          \
} while (0)

#define TAILQ_EMPTY(head) ((head)->tqh_first == NULL)

#define TAILQ_FIRST(head) ((head)->tqh_first)

#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define TAILQ_LAST(head, headname)                                      \
        (*(((struct headname *)((head)->tqh_last))->tqh_last))

#define TAILQ_PREV(elm, headname, field)                                \
        (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#define TAILQ_FOREACH(var, head, field)                                 \
        for ((var) = TAILQ_FIRST((head));                               \
            (var);                                                      \
            (var) = TAILQ_NEXT((var), field))

#define TAILQ_FOREACH_REVERSE(var, head, headname, field)               \
        for ((var) = TAILQ_LAST((head), headname);                      \
            (var);                                                      \
            (var) = TAILQ_PREV((var), headname, field))

#define TAILQ_INSERT_HEAD(head, elm, field) do {                        \
        if ((TAILQ_NEXT((elm), field) = TAILQ_FIRST((head))) != NULL)   \
                TAILQ_FIRST((head))->field.tqe_prev =                   \
                    &TAILQ_NEXT((elm), field);                          \
        else                                                            \
                (head)->tqh_last = &TAILQ_NEXT((elm), field);           \
        TAILQ_FIRST((head)) = (elm);                                    \
        (elm)->field.tqe_prev = &(head)->tqh_first;                     \
} while (0)

#define TAILQ_INSERT_TAIL(head, elm, field) do {                        \
        TAILQ_NEXT((elm), field) = NULL;                                \
        (elm)->field.tqe_prev = (head)->tqh_last;                       \
        *(head)->tqh_last = (elm);                                      \
        (head)->tqh_last = &TAILQ_NEXT((elm), field);                   \
} while (0)

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) do {              \
        if ((TAILQ_NEXT((elm), field) = TAILQ_NEXT((listelm), field))   \
            != NULL)                                                    \
                TAILQ_NEXT((elm), field)->field.tqe_prev =              \
                    &TAILQ_NEXT((elm), field);                          \
        else                                                            \
                (head)->tqh_last = &TAILQ_NEXT((elm), field);           \
        TAILQ_NEXT((listelm), field) = (elm);                           \
        (elm)->field.tqe_prev = &TAILQ_NEXT((listelm), field);          \
} while (0)

#define TAILQ_INSERT_BEFORE(listelm, elm, field) do {                   \
        (elm)->field.tqe_prev = (listelm)->field.tqe_prev;              \
        TAILQ_NEXT((elm), field) = (listelm);                           \
        *(listelm)->field.tqe_prev = (elm);                             \
        (listelm)->field.tqe_prev = &TAILQ_NEXT((elm), field);          \
} while (0)

#define TAILQ_REMOVE(head, elm, field) do {                             \
        if ((TAILQ_NEXT((elm), field)) != NULL)                         \
                TAILQ_NEXT((elm), field)->field.tqe_prev =              \
                    (elm)->field.tqe_prev;                              \
        else                                                            \
                (head)->tqh_last = (elm)->field.tqe_prev;               \
        *(elm)->field.tqe_prev = TAILQ_NEXT((elm), field);              \
} while (0)

#endif /* _SYS_QUEUE_H_ */
