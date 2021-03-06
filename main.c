#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

//Referencias:
//Notas, ejemplos y laboratorios de clase
//https://www.youtube.com/watch?v=YC61729PThw

#define CAMPOS_ESPERA 3
long espera[CAMPOS_ESPERA] = {0, 0, 0}; //Lista de espera (sillas)
long idEstAten = 0;                     //Estudiante que actualmente se está atendiendo (nunca es 0)
int cantEst = 0;                        //Cantidad de estudiantes
int timeA = 0;                          //Tiempo de atención (horario de atencion)
bool siestaBandera = false;
sem_t siesta;

pthread_mutex_t esperaMutex = PTHREAD_MUTEX_INITIALIZER;   //Controla el acceso a la lista de espera
pthread_mutex_t atencionMutex = PTHREAD_MUTEX_INITIALIZER; //Controla el tiempo de atención para sincronizar
pthread_mutex_t vecesMutex = PTHREAD_MUTEX_INITIALIZER;    //Controla el acceso a la disminución de veces que se ejecuta (horario de atencion)
pthread_mutex_t cantEstMutex = PTHREAD_MUTEX_INITIALIZER; 

//Verifica si hay estudiantes esperando
//Nunca un id es 0, por lo que no existe un estudiante con ese id
bool comprobarEspera()
{
    pthread_mutex_lock(&esperaMutex);
    int est = espera[0];
    pthread_mutex_unlock(&esperaMutex);
   
    if (est!= 0){

        return true;
    }
   
    return false;
    
}

//Retorna el estudiante que esta al inicio y corre los campos
//Retorna el id del estudiante (nunca un id es 0)
int obtenerEstAtend()
{
    pthread_mutex_lock(&esperaMutex);
    int tmp = espera[0];
    pthread_mutex_unlock(&esperaMutex);

    for (int i = 0; i < 2; i++){
        pthread_mutex_lock(&esperaMutex);
        espera[i] = espera[i + 1];
        pthread_mutex_unlock(&esperaMutex);
    }

    pthread_mutex_lock(&esperaMutex);
    espera[2] = 0;
    pthread_mutex_unlock(&esperaMutex);
    
    return tmp;
}

//Se coloca al estudiante en la primer posicion, puesto que este despierta al asistente.
bool despertar(long idEst)
{
    pthread_mutex_lock(&esperaMutex);
    espera[0] = idEst;
    pthread_mutex_unlock(&esperaMutex);

    sem_wait(&siesta);
    siestaBandera = false;
    sem_post(&siesta);

    printf("Estudiante %ld despertando a asistente...\n", idEst);

    return true;
}

//Un estudiante busca la atención
//Se coloca en espera en la posición más cercana si hay espacio
//Si el asistente está duermiendo, lo despierta
bool buscAtenc(long idEst)
{
    sem_wait(&siesta);
    bool bandAux = siestaBandera;
    sem_post(&siesta);

    if (!bandAux)
    {
       
        for (int i = 0; i < 3; i++)
        {
            pthread_mutex_lock(&esperaMutex);
            int est = espera[i];
            pthread_mutex_unlock(&esperaMutex);

            if (est == 0)
            {

                pthread_mutex_lock(&esperaMutex);
                espera[i] = idEst;
                pthread_mutex_unlock(&esperaMutex);
                return true;
            }
           
        }
        return false;
    }
    else
    {
        
        return despertar(idEst);
    }
}

//Retorna el tiempo que será atendido el estudiante (random)
int tiemAten()
{
    srand(time(NULL));
    return 5 + rand() % 10;
}

//Rifa quien sera el proximo en ser atendido (random id de estudiantes)
long rifaAten()
{
    srand(time(NULL));

    pthread_mutex_lock(&cantEstMutex);
    int can = cantEst;
    pthread_mutex_unlock(&cantEstMutex);

    return (long)1 + rand() % can;
}

void *thread_asistente()
{
    int i = 1;

    pthread_mutex_lock(&vecesMutex);
    int tiempo = timeA;
    pthread_mutex_unlock(&vecesMutex);

    while (tiempo > 0)
    {

        if (comprobarEspera()) //Si hay un estudiante esperando
        {

            pthread_mutex_lock(&atencionMutex);
            idEstAten = obtenerEstAtend(); //Lo obtiene y se atiende
            pthread_mutex_unlock(&atencionMutex);

            pthread_mutex_lock(&atencionMutex);
            printf("Asistente inicia atencion a estudiante %ld...\n", idEstAten);
            pthread_mutex_unlock(&atencionMutex);

            pthread_mutex_lock(&atencionMutex);
            sleep(tiemAten()); //Bloquea para que el estudiante no siga.
            pthread_mutex_unlock(&atencionMutex);

            pthread_mutex_lock(&atencionMutex);
            printf("Asistente finaliza atencion a estudiante %ld...\n", idEstAten);
            pthread_mutex_unlock(&atencionMutex);

            pthread_mutex_lock(&atencionMutex);
            idEstAten = 0;
            pthread_mutex_unlock(&atencionMutex);

            sleep(0.1);
            
        }
        else
        {
            //Pone a dormir al asistente
            sem_wait(&siesta);
            siestaBandera = true;
            sem_post(&siesta);

            printf("Asistente va a tomar una siesta...\n");

            sem_wait(&siesta);
            bool bandAux = siestaBandera;
            sem_post(&siesta);

            while (bandAux && tiempo > 0)
            {
                sleep(0.1);

                sem_wait(&siesta);
                bandAux = siestaBandera;
                sem_post(&siesta);

                pthread_mutex_lock(&vecesMutex);
                tiempo = timeA;
                pthread_mutex_unlock(&vecesMutex);

            }

      }
        pthread_mutex_lock(&vecesMutex);
        tiempo = timeA;
        pthread_mutex_unlock(&vecesMutex);

    }
    printf("El horario de atencion ha finalizado...\n");
}

void *thread_estudiante(void *idPtr)
{
    long id = (long)idPtr;
    int i = 1;

    pthread_mutex_lock(&vecesMutex);
    int tiempo = timeA;
    pthread_mutex_unlock(&vecesMutex);

    while (tiempo > 0)
    {

        printf("Estudiante %ld programando...\n", id);

        //sleep(2);

        if (rifaAten() == id) //Se realiza la rifa para buscar atencion
        {
            printf("Estudiante %ld buscando atencion...\n", id);
            if (buscAtenc(id)) //Se pone en cola para la atencion si hay campo
            {
                //Puede que en el momento que se ponga en cola no esté siendo atendido, espera a serlo.
                //Por lo que mientras que el estudiante que se esté atendiendo sea diferente de el(idEstAten)
                //Va a esperar siempre y cuando haya tiempo para atenderse

                pthread_mutex_lock(&atencionMutex);
                int estAt = idEstAten;
                pthread_mutex_unlock(&atencionMutex);

                while (estAt != id && tiempo > 0)
                {

                    printf("Estudiante %ld esperando atencion...\n", id);
                    //sleep(5);

                    pthread_mutex_lock(&vecesMutex);
                    tiempo = timeA;
                    pthread_mutex_unlock(&vecesMutex);

                    pthread_mutex_lock(&atencionMutex);
                    estAt = idEstAten;
                    pthread_mutex_unlock(&atencionMutex);

                    sleep(5);

                }

                pthread_mutex_lock(&atencionMutex);
                //Cuando ya está haciedo atendido termina el bucle, pero debe esperar que el asistente
                //libere el espacio de atención
                pthread_mutex_unlock(&atencionMutex);
            }
            else
            {
                printf("Estudiante %ld regresera mas tarde a buscar atencion...\n", id);
                sleep(1.5);
            }
        } else {
            sleep(1.5);
        }
        
        pthread_mutex_lock(&vecesMutex);
        tiempo = --timeA; //Se disminuye el tiempo de atencion (horario)
        pthread_mutex_unlock(&vecesMutex);
    }
}

int main(int argc, char *argv[])
{
    pthread_mutex_lock(&cantEstMutex);
    cantEst = atoi(argv[1]);
    pthread_mutex_unlock(&cantEstMutex);

    pthread_mutex_lock(&cantEstMutex);
    long cantEstAux = (long) cantEst;
    pthread_mutex_unlock(&cantEstMutex);

    pthread_mutex_lock(&vecesMutex);
    timeA = cantEstAux * 6; //Para tener un tiempo de atención aceptable, se hace la operacion
    pthread_mutex_unlock(&vecesMutex);

    pthread_t asistente;
    pthread_t estudiante[cantEstAux];

    int rc;

    sem_init(&siesta, 0, 1);

    rc = pthread_create(&asistente, NULL, thread_asistente, NULL);

    if (rc)
    {
        fprintf(stderr, "ERROR: no se pudo crear el thread asistente\n");
        exit(1);
    }

    for (long i = 0; i < cantEstAux; i++)
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