#include <stdlib.h>
#include <string.h>

typedef struct {
 int capacity;
 int len;
 char *ptr;
}
sbuf_t;


sbuf_t* sbuf_new(int capacity)
{
  char *ptr = (char *)malloc(capacity);
  if (ptr == NULL) return NULL;

  sbuf_t *obj = (sbuf_t *)malloc(sizeof(sbuf_t));
  if (obj == NULL) return NULL;

  obj->ptr = ptr;
  obj->capacity = capacity;
  obj->len = 0;

  return obj;
}

int sbuf_remaining(sbuf_t *buf)
{
  return (buf->capacity - buf->len - 1);  // 1-byte zero terminator
}

// return 0 on success, 1 otherwise (not able to allocate)
int sbuf_resize(sbuf_t *buf, int capacity)
{
  char *ptr = (char *)malloc(capacity);
  if (ptr == NULL) return 1;
  
  strncpy(ptr, buf->ptr, buf->len);
  free(buf->ptr);
  buf->ptr = ptr;
  buf->capacity = capacity;

  return 0;
}

// return 0 on success, 1 otherwise (not enough space)
int sbuf_append(sbuf_t *buf, char *str)
{
  int len_to_add = strlen(str);

  if (len_to_add > sbuf_remaining(buf)) {
    if (sbuf_resize(buf, buf->capacity * 2) != 0) return 1;
  }

  strcpy(buf->ptr + buf->len, str);
  buf->len += len_to_add;

  return 0;
}

char *sbuf_to_s(sbuf_t *buf)
{
  return buf->ptr;
}

