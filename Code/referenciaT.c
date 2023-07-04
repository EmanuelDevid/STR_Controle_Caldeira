/* Monitor sensor, no arquivo sensor.c */
#include <math.h>
#include <pthread.h>

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER;
static double ref_entradaT = 0;
static double ref_entradaN = 0;

/* Chamado pela thread que le o sensor e disponibiliza aqui o valor lido */
void ref_putT(double ref)
{
	pthread_mutex_lock(&exclusao_mutua);
	ref_entradaT = ref;
	pthread_mutex_unlock(&exclusao_mutua);
}

/* Chamado por qualquer thread que precisa do valor lido do sensor */
double ref_getT(void)
{
	double aux;
	pthread_mutex_lock(&exclusao_mutua);
	aux = ref_entradaT;
	pthread_mutex_unlock(&exclusao_mutua);
	return aux;
}

/* Chamado pela thread que le o sensor e disponibiliza aqui o valor lido */
void ref_putN(double ref)
{
	pthread_mutex_lock(&exclusao_mutua);
	ref_entradaN = ref;
	pthread_mutex_unlock(&exclusao_mutua);
}

/* Chamado por qualquer thread que precisa do valor lido do sensor */
double ref_getN(void)
{
	double aux;
	pthread_mutex_lock(&exclusao_mutua);
	aux = ref_entradaN;
	pthread_mutex_unlock(&exclusao_mutua);
	return aux;
}