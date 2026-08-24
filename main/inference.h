#ifndef INFERENCE_H
#define INFERENCE_H

#ifdef __cplusplus
extern "C" {
#endif

#define MFCC_SIZE 13*49

void inference_init(void);
void inference_start(void);

#ifdef __cplusplus
}
#endif

#endif