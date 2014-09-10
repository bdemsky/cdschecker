#ifndef _MODEL_MEMORY_H
#define _MODEL_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

void* snapshot_malloc(size_t size);
void* snapshot_calloc(size_t count, size_t size);
void snapshot_free(void *ptr);

#define MODEL_MALLOC(x) snapshot_malloc((x))
#define MODEL_CALLOC(x, y) snapshot_calloc((x), (y))
#define MODEL_FREE(x) snapshot_free((x))

#define CMODEL_MALLOC(x) snapshot_malloc((x))
#define CMODEL_CALLOC(x, y) snapshot_calloc((x), (y))
#define CMODEL_FREE(x) snapshot_free((x))

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif
