#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#define CAMPOS_ESPERA 3
long espera[CAMPOS_ESPERA] = {0, 0, 0};
long idEstAten = 0;
int cantEst = 0;
int timeA = 0;

pthread_mutex_t esperaMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t atencionMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t vecesMutex = PTHREAD_MUTEX_INITIALIZER;

bool comprobarEspera()
{
    if (espera[0] != 0)
        return true;
    return false;
}

int obtenerEstAtend()
{
    pthread_mutex_lock(&esperaMutex);
    int tmp = espera[0];
    for (int i = 0; i < 2; i++)
        espera[i] = espera[i + 1];
    espera[2] = 0;
    pthread_mutex_unlock(&esperaMutex);
    return tmp;
}

bool buscAtenc(long idEst)
{
    pthread_mutex_lock(&esperaMutex);
    for (int i = 0; i < 3; i++)
    {
        if (espera[i] == 0)
        {
            espera[i] = idEst;
            pthread_mutex_unlock(&esperaMutex);
            return true;
        }
    }
    pthread_mutex_unlock(&esperaMutex);
    return false;
}

int tiemAten()
{
    srand(time(NULL));
    return 5 + rand() % 10;
}

long rifaAten()
{
    srand(time(NULL));
    return (long)1 + rand() % cantEst;
}

void *thread_asistente()
{
    while (timeA > 0)
    {
        if (comprobarEspera())
        {
            pthread_mutex_lock(&atencionMutex);
            idEstAten = obtenerEstAtend();
            printf("Asistente inicia atencion a estudiante %ld...\n", idEstAten);
            sleep(tiemAten());
            pthread_mutex_unlock(&atencionMutex);
            printf("Asistente finaliza atencion a estudiante %ld...\n", idEstAten);
        }
        else
        {
            //duerme
        }
    }
    printf("El horario de atencion ha finalizado...\n");
}

void *thread_estudiante(void *idPtr)
{
    long id = (long)idPtr;

    while (timeA > 0)
    {

        printf("Estudiante %ld programando...\n", id);

        if (rifaAten() == id)
        {
            printf("Estudiante %ld buscando atencion...\n", id);
            if (buscAtenc(id))
            {
                while (idEstAten != id && timeA > 0)
                {
                    printf("Estudiante %ld esperando atencion...\n", id);
                    sleep(5);
                }
                pthread_mutex_lock(&atencionMutex);

                pthread_mutex_unlock(&atencionMutex);
            }
            else
            {
                printf("Estudiante %ld regresera mas tarde a buscar atencion...\n", id);
            }
        }
        sleep(1.5);
        pthread_mutex_lock(&vecesMutex);
        timeA--;
        pthread_mutex_unlock(&vecesMutex);
    }
}

int main(int argc, char *argv[])
{
    cantEst = atoi(argv[1]);
    timeA = cantEst * 6;
    pthread_t asistente;
    pthread_t estudiante[cantEst];

    int rc;

    rc = pthread_create(&asistente, NULL, thread_asistente, NULL);

    if (rc)
    {
        fprintf(stderr, "ERROR: no se pudo crear el thread asistente\n");
        exit(1);
    }

    for (long i = 0; i < cantEst; i++)
    {
        rc = pthread_create(&estudiante[i], NULL, thread_estudiante, (void *)i + 1);

        if (rc)
        {
            fprintf(stderr, "ERROR: no se pudo crear el thread estudiante\n");
            exit(1);
        }
    }
    pthread_exit(NULL);
}