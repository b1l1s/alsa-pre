#include <alsa/asoundlib.h>

#define __USE_GNU
#include <dlfcn.h>

// https://rafalcieslak.wordpress.com/2013/04/02/dynamic-linker-tricks-using-ld_preload-to-cheat-inject-features-and-investigate-programs/

// LD_PRELOAD=./libalsa-pre.so.1.0.0 aplay test.wav
// LD_DEBUG=symbols LD_PRELOAD=./libalsa-pre.so.1.0.0 aplay test.wav

/**
 * Generate typedefs and function pointers as follows
 * typedef int (*orig_snd_pcm_prepare_f_type)(snd_pcm_t *pcm);
 * static orig_snd_pcm_prepare_f_type orig_snd_pcm_prepare = NULL;
 */
#define CR(name, ret, sig) typedef ret (*orig_ ## name ## _f_type)sig; \
                            static orig_ ## name ## _f_type orig_ ## name = NULL;
/**
 * Get symbol as follows
 * orig_snd_pcm_prepare = (orig_snd_pcm_prepare_f_type) dlsym(RTLD_NEXT, "snd_pcm_prepare");
 */
#define CV(name) orig_ ## name = (orig_ ## name ## _f_type) dlsym(RTLD_NEXT, #name);

CR(snd_pcm_prepare, int, (snd_pcm_t *pcm))
CR(snd_pcm_wait, int, (snd_pcm_t *pcm, int timeout))
CR(snd_pcm_sw_params, int, (snd_pcm_t *pcm, snd_pcm_sw_params_t *params))
CR(snd_pcm_hw_params, int, (snd_pcm_t *pcm, snd_pcm_hw_params_t *params))

static snd_pcm_status_t *status = NULL;
static snd_output_t *output = NULL;

static void init() __attribute__((constructor));
void init()
{
    CV(snd_pcm_prepare);
    CV(snd_pcm_wait);
    CV(snd_pcm_sw_params);
    CV(snd_pcm_hw_params);
    
    snd_output_stdio_attach(&output, stdout, 0);
    snd_pcm_status_malloc(&status);
}

int snd_pcm_wait(snd_pcm_t *pcm, int timeout)
{
    printf("\nsnd_pcm_wait\n");
    
    int ret = orig_snd_pcm_wait(pcm, timeout);
    if (ret <= 0)
    {
        snd_pcm_status(pcm, status);
    }

    return orig_snd_pcm_wait(pcm, timeout);
}

int snd_pcm_prepare(snd_pcm_t *pcm)
{
    printf("\nsnd_pcm_prepare\n");
    snd_pcm_dump(pcm, output);
    //snd_pcm_dump_hw_setup(pcm, output);
    //snd_pcm_dump_sw_setup(pcm, output);
    return orig_snd_pcm_prepare(pcm);
}

int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
    printf("\nsnd_pcm_sw_params\n");

    snd_pcm_sw_params_dump(params, output);
    return orig_snd_pcm_sw_params(pcm, params);
}

int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    printf("\nsnd_pcm_hw_params\n");

    snd_pcm_hw_params_dump(params, output);
    return orig_snd_pcm_hw_params(pcm, params);
}