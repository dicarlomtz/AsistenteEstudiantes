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
pthread_mutex_t duranteAtendMutex = PTHREAD_MUTEX_INITIALIZER; 

//Verifica si hay estudiantes esperando
//Nunca un id es 0, por lo que no existe un estudiante con ese id
bool comprobarEspera()
{   

    if (espera[0] != 0)
        return true;
    return false;
    
}

//Retorna el estudiante que esta al inicio y corre los campos
//Retorna el id del estudiante (nunca un id es 0)
long obtenerEstAtend()
{
    long tmp = espera[0];

    espera[0] = espera[1];
    espera[1] = espera[2];
    espera[2] = 0; 
    
    return tmp;
}

//Se coloca al estudiante en la primer posicion, puesto que este despierta al asistente.
bool despertar(long idEst)
{
    espera[0] = idEst;
    
    siestaBandera = false;

    sleep(0.5);
    printf("Estudiante %ld despertando a asistente...\n", idEst);

    return true;
}

//Un estudiante busca la atención
//Se coloca en espera en la posición más cercana si hay espacio
//Si el asistente está duermiendo, lo despierta
bool buscAtenc(long idEst)
{

    if (!siestaBandera)
    {
       
        for (int i = 0; i < 3; i++)
        {

            if (espera[i] == 0)
            {
                espera[i] = idEst;
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
    return 3 + rand() % 7;
}

//Rifa quien sera el proximo en ser atendido (random id de estudiantes)
long rifaAten()
{
    srand(time(NULL));

    return (long)1 + rand() % cantEst;
}

void *thread_asistente()
{

    while (1)
    {

        pthread_mutex_lock(&vecesMutex);
        if(timeA <= 0){
            pthread_mutex_unlock(&vecesMutex);
            break;
        }
        pthread_mutex_unlock(&vecesMutex);

        pthread_mutex_lock(&esperaMutex);
        if (comprobarEspera()) //Si hay un estudiante esperando
        {

            int est = obtenerEstAtend();
            pthread_mutex_unlock(&esperaMutex); //Se libera espera mutex

            pthread_mutex_lock(&duranteAtendMutex); //Se hace lock para que el thread no continue hasta terminar el tiempo de atencion

            pthread_mutex_lock(&atencionMutex);
            idEstAten = est; //Lo obtiene y se atiende
            pthread_mutex_unlock(&atencionMutex); // Se libera el lock de atencion para asegurar que los thread tengan acceso a el id actualzado

            pthread_mutex_lock(&atencionMutex); //Se hace lock de antencion dentro para asegurar que los print de aca se ejecuten antes de que el thread en atención continue
            sleep(0.5);
            printf("Asistente inicia atencion a estudiante %ld...\n", idEstAten);
            pthread_mutex_unlock(&atencionMutex);

            sleep(tiemAten()); //Espera para que el estudiante no siga. (tiempo de atencion)

            pthread_mutex_lock(&atencionMutex); //Se hace lock de antencion dentro para asegurar que los print de aca se ejecuten antes de que el thread en atención continue
            sleep(0.5);
            printf("Asistente finaliza atencion a estudiante %ld...\n", idEstAten);
            pthread_mutex_unlock(&atencionMutex);

            pthread_mutex_unlock(&duranteAtendMutex);
            

            sleep(2);
            
        }
        else
        {
            pthread_mutex_unlock(&esperaMutex);

            //Pone a dormir al asistente
            sem_wait(&siesta);
            siestaBandera = true;
            sem_post(&siesta);

            printf("Asistente va a tomar una siesta...\n");

            while (1)
            {
                
                pthread_mutex_lock(&vecesMutex);
                sem_wait(&siesta);
                if(!siestaBandera || timeA <= 0){
                    sem_post(&siesta);
                    pthread_mutex_unlock(&vecesMutex);
                    break;
                }
                sem_post(&siesta);
                pthread_mutex_unlock(&vecesMutex);

            }

      }

    }
    printf("El horario de atencion ha finalizado...\n");
}

void *thread_estudiante(void *idPtr)
{
    long id = (long)idPtr;

    while (1)
    {

        pthread_mutex_lock(&vecesMutex);
        if(timeA <= 0){
            pthread_mutex_unlock(&vecesMutex);
            break;
        }
        pthread_mutex_unlock(&vecesMutex);

        sleep(0.5);
        printf("Estudiante %ld programando...\n", id);

        pthread_mutex_lock(&cantEstMutex);
        if (rifaAten() == id) //Se realiza la rifa para buscar atencion
        {
            pthread_mutex_unlock(&cantEstMutex);

            sleep(0.5);
            printf("Estudiante %ld buscando atencion...\n", id);

            pthread_mutex_lock(&esperaMutex);
            sem_wait(&siesta);
            if (buscAtenc(id)) //Se pone en cola para la atencion si hay campo
            {
                sem_post(&siesta);
                pthread_mutex_unlock(&esperaMutex);
                  
                //Puede que en el momento que se ponga en cola no esté siendo atendido, espera a serlo.
                //Por lo que mientras que el estudiante que se esté atendiendo sea diferente de el(idEstAten)
                //Va a esperar siempre y cuando haya tiempo para atenderse

                while(1) { 

                    pthread_mutex_lock(&vecesMutex);
                    pthread_mutex_lock(&atencionMutex);
                    if(idEstAten == id || timeA <= 0){

                        pthread_mutex_unlock(&atencionMutex);
                        pthread_mutex_unlock(&vecesMutex);

                        break;

                    }
                    pthread_mutex_unlock(&atencionMutex);
                    pthread_mutex_unlock(&vecesMutex);

                }

                pthread_mutex_lock(&duranteAtendMutex);
                //Cuando ya está haciedo atendido termina el bucle, pero debe esperar que el asistente
                //libere el espacio de atención.
                pthread_mutex_unlock(&duranteAtendMutex);
            }
            else
            {
                sem_post(&siesta);
                pthread_mutex_unlock(&esperaMutex);
                
                sleep(0.5);
                printf("Estudiante %ld regresara mas tarde a buscar atencion...\n", id);
            }
        } else {
            pthread_mutex_unlock(&cantEstMutex);
        }

        sleep(1.5);
        pthread_mutex_lock(&vecesMutex);
        timeA--; //Se disminuye el tiempo de atencion (horario)
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