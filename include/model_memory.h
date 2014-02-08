#ifndef _MODEL_MEMORY_H
#define _MODEL_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

void* model_malloc(size_t size);
void* model_calloc(size_t count, size_t size);
void model_free(void *ptr);

#define MODEL_MALLOC(x) model_malloc((x))
#define MODEL_CALLOC(x, y) model_calloc((x), (y))
#define MODEL_FREE(x) model_free((x))

#define CMODEL_MALLOC(x) model_malloc((x))
#define CMODEL_CALLOC(x, y) model_calloc((x), (y))
#define CMODEL_FREE(x) model_free((x))

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif
