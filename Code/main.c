// Defini��o de Bibliotecas
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "socket.h"
#include "sensores.h"
#include "tela.h"
#include "bufduplo.h"
#include "referenciaT.h"

#define NSEC_PER_SEC (1000000000) // Numero de nanosegundos em um segundo

#define N_AMOSTRAS 10000

void thread_mostra_status(void)
{
	double t, h;
	while (1)
	{
		t = sensor_get("t");
		h = sensor_get("h");
		aloca_tela();
		system("tput reset");
		printf("---------------------------------------\n");
		printf("Temperatura (T)--> %.2lf\n", t);
		printf("Nivel       (H)--> %.2lf\n", h);
		printf("---------------------------------------\n");
		libera_tela();
		sleep(1);
		//
	}
}

void thread_le_sensor(void)
{ // Le Sensores periodicamente a cada 10ms
	struct timespec t;
	long periodo = 10e6; // 10e6ns ou 10ms

	// Le a hora atual, coloca em t
	clock_gettime(CLOCK_MONOTONIC, &t);
	while (1)
	{
		// Espera ateh inicio do proximo periodo
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

		// Envia mensagem via canal de comunica��o para receber valores de sensores
		// Envia mensagem via canal de comunicação para receber valores de sensores
		sensor_put(msg_socket("st-0"), msg_socket("sh-0"), msg_socket("sta0"), msg_socket("sti0"), msg_socket("sno0"));

		// Calcula inicio do proximo periodo
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC)
		{
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}
	}
}

void thread_alarme(void)
{
	while (1)
	{
		sensor_alarmeT(29);
		aloca_tela();
		printf("ALARME\n");
		libera_tela();
		sleep(1);
	}
}

// Controle Temperatura
void thread_controle_temperatura(void)
{
	char msg_enviada[1000];
	long atraso_fim;
	struct timespec t, t_fim; // tempo
	long periodo = 50e6;	  // 50ms
	double temp, ref_temp;
	// Le a hora atual, coloca em t
	clock_gettime(CLOCK_MONOTONIC, &t);
	t.tv_sec++;
	while (1)
	{

		// Espera ateh inicio do proximo periodo
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

		temp = sensor_get("t");
		ref_temp = ref_getT();
		double na;

		if (temp > ref_temp)
		{ // diminui temperatura

			sprintf(msg_enviada, "ani%lf", 100.0); // entrada agua fria
			msg_socket(msg_enviada);

			sprintf(msg_enviada, "anf%lf", 100.0); // fluxo de saída do esgoto
			msg_socket(msg_enviada);

			sprintf(msg_enviada, "ana%lf", 0.0); // fluxo de agua quente entrando
			msg_socket(msg_enviada);
		}

		if (temp <= ref_temp)
		{ // aumenta temperatura

			if ((ref_temp - temp) * 20 > 10.0) // ajustes grandes
				na = 10.0;					   // quanto mais se aproxima do objetivo de temperatura, menor será a entrada de agua quente Na
			else
				na = (ref_temp - temp) * 20; // ajustos finos

			sprintf(msg_enviada, "ani%lf", 0.0);
			msg_socket(msg_enviada);

			sprintf(msg_enviada, "anf%lf", 10.0);
			msg_socket(msg_enviada);

			sprintf(msg_enviada, "ana%lf", na);
			msg_socket(msg_enviada);
		}

		// Le a hora atual, coloca em t_fim
		clock_gettime(CLOCK_MONOTONIC, &t_fim);

		// Calcula o tempo de resposta observado em microsegundos
		atraso_fim = 1000000 * (t_fim.tv_sec - t.tv_sec) + (t_fim.tv_nsec - t.tv_nsec) / 1000;

		bufduplo_insereLeitura(atraso_fim);

		// Calcula inicio do proximo periodo
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC)
		{
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}
	}
}

// Controle Nível
void thread_controle_nivel(void)
{
	char msg_enviada[1000];
	long atraso_fim;
	struct timespec t, t_fim;
	long periodo = 10e6; // 50ms
	double nivel, ref_nivel, no, ti, na, ni, temp;

	// Le a hora atual, coloca em t
	clock_gettime(CLOCK_MONOTONIC, &t);
	t.tv_sec++;
	while (1)
	{
		// Espera até o início do próximo período
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

		nivel = sensor_get("h");
		ref_nivel = ref_getN();
		no = sensor_get("no");
		// ti = sensor_get("ti");

		if (nivel > ref_nivel)
		{ // Diminuir nível
			sprintf(msg_enviada, "anf%lf", 100.0);
			msg_socket(msg_enviada);

			sprintf(msg_enviada, "ana%lf", 0.0);
			msg_socket(msg_enviada);

			sprintf(msg_enviada, "ani%lf", 0.0);
			msg_socket(msg_enviada);
		}

		if (nivel < ref_nivel)
		{ // Aumentar nível
			sprintf(msg_enviada, "ana%lf", 150.0);
			msg_socket(msg_enviada);

			sprintf(msg_enviada, "anf%lf", 0.0);
			msg_socket(msg_enviada);

			sprintf(msg_enviada, "ani%lf", 0.0);
			msg_socket(msg_enviada);
		}

		// Le a hora atual, coloca em t_fim
		clock_gettime(CLOCK_MONOTONIC, &t_fim);

		// Calcula o tempo de resposta observado em microssegundos
		atraso_fim = 1000000 * (t_fim.tv_sec - t.tv_sec) + (t_fim.tv_nsec - t.tv_nsec) / 1000;

		bufduplo_insereLeitura(atraso_fim);

		// Calcula o início do próximo período
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC)
		{
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}
	}

	pthread_exit(NULL);
}

void thread_grava_temp_resp(void)
{
	FILE *dados_f;
	dados_f = fopen("temperatura.txt", "w");
	if (dados_f == NULL)
	{
		printf("Erro, nao foi possivel abrir o arquivo\n");
		exit(1);
	}
	int amostras = 1;
	while (amostras++ <= N_AMOSTRAS / 200)
	{
		long *buf = bufduplo_esperaBufferCheio();
		int n2 = tamBuf();
		int tam = 0;
		while (tam < n2)
			fprintf(dados_f, "%4ld\n", buf[tam++]);
		fflush(dados_f);
		aloca_tela();
		printf("Gravando no arquivo...\n");

		libera_tela();
	}

	fclose(dados_f);
}

void thread_gravar_nivel(void)
{
	FILE *dados_nivel;
	dados_nivel = fopen("nivel.txt", "w");

	if (dados_nivel == NULL)
	{
		printf("Erro, nao foi possivel abrir o arquivo nivel.txt\n");
		exit(1);
	}

	int amostras = 1;
	while (amostras++ <= N_AMOSTRAS / 200)
	{
		long *buf = bufduplo_esperaBufferCheio();
		int n2 = tamBuf();
		int tam = 0;
		while (tam < n2)
			fprintf(dados_nivel, "%4ld\n", buf[tam++]);
		fflush(dados_nivel);
		aloca_tela();
		printf("Gravando no arquivo nivel...\n");

		libera_tela();
	}

	fclose(dados_nivel);
}

int main(int argc, char *argv[])
{
	ref_putT(29.0);
	ref_putN(2.0);
	cria_socket(argv[1], atoi(argv[2]));

	pthread_t t1, t2, t3, t4, t5, t6, t7;

	pthread_create(&t1, NULL, (void *)thread_mostra_status, NULL);
	pthread_create(&t2, NULL, (void *)thread_le_sensor, NULL);
	pthread_create(&t3, NULL, (void *)thread_alarme, NULL);
	pthread_create(&t4, NULL, (void *)thread_controle_temperatura, NULL);
	pthread_create(&t5, NULL, (void *)thread_grava_temp_resp, NULL);
	pthread_create(&t6, NULL, (void *)thread_controle_nivel, NULL);
	pthread_create(&t7, NULL, (void *)thread_gravar_nivel, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);
	pthread_join(t4, NULL);
	pthread_join(t5, NULL);
	pthread_join(t6, NULL);
	pthread_join(t7, NULL);

	return 0;
}
