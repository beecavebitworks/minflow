#ifndef _STRINGBUF_H_
#define _STRINGBUF_H_

struct sbuf_t;
typedef struct sbuf_t sbuf_t;

sbuf_t *sbuf_new(int size);

void sbuf_append(sbuf_t *buf, char *str);

char *sbuf_to_s(sbuf_t *buf);

#endif // _STRINGBUF_H_
