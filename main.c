#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

//Numero de hijos a crear
int cantidadHijos;
//Numero de llamadas por hijo
int contador_llamadas;
//PID del padre inicial
int pid_padre;
//Variable de condicion SIGINT
int bIstime;


//Lista enlazada usada para almacenar a los hijos creados por el padre y la cantidad de veces que este fue llamado
struct _hijos {
  int pid;
  int llamadas;
  struct _hijos *siguiente;
};

//Punteros para la lista enlazada
struct _hijos *primero, *ultimo;

//Sleep para mantener ocupado algun proceso
void do_nothing(){
  sleep(1); //1 second
}


//Declacación de funciones
void enviar_signal(int numero_hijo,int numeroSenal);
void crearHijos(int numero_hijos_crear,int condicion);
void crearNieto();
void padre_listening(int numero_hijos_crear);

//Funcion para señales
//Esta función es el controlador de las señales, la cual recibe como parametro un entero 
//el cual indica que tipo de señal a sido enviada ya sea por consola o teclado

void get_signal(int signum)
{
  switch(signum)
  {
    //Señal SIGUSR1, esta señal imprime por pantalla la cantidad de veces que el hijo de X PID a sido llamado
    //por esta señal
    case SIGUSR1:
      //contador_llamadas solamente se aumenta en el hijo
      contador_llamadas++;
      printf("pid: %d y he recibido esta llamada %d veces\n",getpid(),contador_llamadas);
      break;
    //Señal SIGTERM, esta señal mata con la señal SIGTERM al proceso que a llamado a esta señal
    case SIGTERM:
      kill(getpid(),SIGKILL);
      break;
    //Señal SIGURS2, esta señal llama la función crearNieto, la cual se encarga de crear los hijos de los hijos
    // del padre inicial
    case SIGUSR2:
      crearNieto();
      break;
    //Señal SIGINT, esta señal es activada al momento de presionar ctrl+c. Cuando es presionada por primera vez
    //la señal procedera a imprimir por pantalla a todos los hijos del padre indicando si estan vivos o estan muertos.
    //Para la segunda vez envia la señal SIGKILL a todos los procesos, tanto padre como hijos
    case SIGINT:
      if(bIstime > 0 && getpid() == pid_padre)
      {
        kill(getpid(),SIGKILL);
      }
      else if(bIstime > 0 && getpid() != pid_padre)
      {
        kill(getpid(),SIGKILL);
      }
      if(getpid() != pid_padre && getppid() == pid_padre)
      {
        printf("Soy el hijo con %d y estoy vivo aun \n", getpid());
        bIstime++;
      }
      else
      {
        bIstime++;
      }
      break;
  }

}

//Al momento de crear un hijo se le asignan las señales 15, 16 y 17 que corresponden a
//SIGTERM, SIGUSR1 y SIGUSR2, lo cual permite redefinir la función de las señales
void iniciar_hijo()
{
  //Se asigna cantidadHijos a cero para que cada hijo pueda motrar la cantidad de hijos que tiene
  cantidadHijos = 0;
  signal(SIGTERM, get_signal);
  signal(SIGUSR1, get_signal);
  signal(SIGUSR2, get_signal);

  //Se mantiene al hijo vivo
  while(1)
  {
    //do_nothing();
  }
}



//Función usada por el proceso padre
//La cual tiene como argumentos 2 enteros, siendo el primero usado para identificar a cual hijo
//se desea acceder, y el segundo argumento indica que tipo de señal sera enviada

void enviar_signal(int numero_hijo,int numeroSenal){

  //Buscamos al hijo 
  int pid_hijo;
  int respueta_kill;
  struct _hijos *auxiliar;
  int i;
  i=1;
  auxiliar = primero;
  if(numero_hijo > 0)
    printf("La señal %d será enviada al hijo %d \n",numeroSenal,numero_hijo);
  else
    printf("La señal %d será enviada a todos los hijos \n",numeroSenal);


  //Ciclo para ubicarse en la lista enlazada con el hijo correspondiente
  while (1) {
            
    if(i == numero_hijo)
    {
      break;
    }
    else
    {
      auxiliar = auxiliar->siguiente;
      i++;
    }
  }
  pid_hijo = auxiliar->pid;

  //Enviamos la signal
  switch(numeroSenal)
  {
    case 16:
      kill(pid_hijo,SIGUSR1);
      break;
    case 15:
      kill(pid_hijo,SIGTERM);
      break;
    case 17:
      kill(pid_hijo,SIGUSR2);
      break;
  }
}

//Función la cual se encarga de mantener al padre en un estado de espera.
//Recibe como parametro la cantidad de hijos que fueron creados exitosamente.
//En esta función se esta preguntando constantemente a que hijo enviar una señal.
//La forma de enviar una señal es ingresando por consola X - Y
//en donde X equivale al numero del hijo que se quiere acceder e Y el tipo de señal que se desea enviar
//Es necesario ingresar el -, ya que esta fue la manera en que se nos fue solicitado
//el ingreso del comando.

void padre_listening(int numero_hijos_crear)
{
  //variables para la  funcion scanf
  int numero_hijo;
  int numeroSenal;
  char repuesto;
  while(1)
  {
    //Recibir las señales por consola
    scanf("%d %c %d",&numero_hijo,&repuesto,&numeroSenal);
    fflush(stdin);
    if(numero_hijo > 0 && numero_hijo <= numero_hijos_crear && numeroSenal >= 15 && numeroSenal <= 17  )
      enviar_signal(numero_hijo,numeroSenal);
    else
      printf("INGRESE UNA OPCION VALIDA\n");
    do_nothing();
  }
}


//Funcion que crea un nodo, para poder mantener registro de los hijos que son creados a partir del padre
//y posteriormente saber cuantas veces es llamado dicho hijo
//Esta función recibe como parametro un entero el cual es el pid de 1 proceso hijo
void add_hijo_arreglo(int pid)
{
  struct _hijos *nuevo;
  nuevo = (struct _hijos *) malloc (sizeof(struct _hijos));
  if (nuevo==NULL) 
    printf( "No hay memoria suficiente\n");

  nuevo->pid = pid;
  nuevo->llamadas = 0;
  nuevo->siguiente = NULL;

  if (primero==NULL) {
     primero = nuevo;
     ultimo = nuevo;
     }
  else {
       ultimo->siguiente = nuevo;
       ultimo = nuevo;
  }
}

//La funcuón crearHijos recibe como parametros la cantidad de hijos que se van a crear y una variable de condicion
//la cual corresponde al flag -m, en la cual de haber existido dicho flag si mostrara por pantalla el numero del hijo creado
//junto con su PID, de no estar activo el flag, los procesos son creados, pero no se muestran por pantalla.

void crearHijos(int numero_hijos_crear,int condicion)
{
  pid_t pid;
  pid_padre = getpid();

  for (int j=0; j < numero_hijos_crear; j++){

    if(getpid() == pid_padre)
    {
      pid = fork();

      if (pid < 0){
        printf("no se pudo crear hijo\n");
        exit(-1);
      }
      if(pid >= 0)
      {
        cantidadHijos++;
      }
      if(getpid() == pid_padre && condicion == 1)
        printf("Número: %d ,pid %d\n",cantidadHijos,pid);

      if(getpid() == pid_padre)
      {
        add_hijo_arreglo((int)pid);
      }
    }
  }
  //SOlO HIJOS
  if(getpid() != pid_padre)
  {
    if(condicion == 1)
      printf("Soy hijo: %d, con pid %d\n",cantidadHijos,getpid());
    iniciar_hijo();
  }
  //Padre queda esperando ordenes
  if(getpid() == pid_padre)
    padre_listening(numero_hijos_crear);
}


//Funcion, que es llamada por la señal SIGUSR2 y se encarga de crear
//un nieto (o hijo del hijo)
void crearNieto()
{
  pid_t pid;
  pid_padre = getpid();
  if(getpid() == pid_padre)
  {
    pid = fork();
    if (pid < 0){
      printf("no se pudo crear nieto\n");
      exit(-1);
    }
    if(pid >= 0)
    {
      cantidadHijos++;
    }
    if(getpid() == pid_padre)
    {
      printf("Número: %d ,pid %d\n",cantidadHijos,pid);
    }
  }
  //SOlO HIJOS
  if(getpid() != pid_padre)
  {
    printf("Soy hijo: %d de mi padre %d y mi pid es %d\n",cantidadHijos,getppid(),getpid());
    while(1)
    {
      //do_nothing();
    }
  }
}


//La función main se encarga principalmente de recibir los datos necesarios para
//conocer cuantos hijos el padre obtendra y si es necesario mostrar la creación de estos hijos.
//Es necesario a la hora de ejecutar el programa que la función venga acompañada de los flags
//-h y -m, en donde -h debe venir acompado de un entero para conocer la cantida de hijos que se deben crear
//y la flag -m de ser enviada permitira al programa mostrar información relevante de los hijos creados.
//No se ser incluida, el programa creara a los hijos, pero sin mostrarlos por pantalla.
int main (int argc, char **argv) {

  char *hvalue = NULL;
  char *mflag = NULL;
  int index,c;
  int var;

  int condicion = 0;


  while ((c = getopt (argc, argv, "h:m")) != -1)
    switch (c)
      {
      //Opcion que indica cuantos hijos quieren que se creen      
      case 'h':
        sscanf(optarg,"%d",&var);
        hvalue = optarg;
        break;      
      //Opcion para mostrar por pantalla todos los hijos activos
      //Que posee el padre
      case 'm':
        mflag = optarg;
        condicion = 1;
        break;
      case '?':
        if (optopt == 'h')
          fprintf (stderr, "Opcion -%c requiere un argumento.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Opcion desconocida `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Opcion con caracter desconocido `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }
  //Cambiamos la señal SIGINT del padre para posteriormente usarla
  signal(SIGINT, get_signal);
  //Creamos los hijos del padre
  crearHijos(var,condicion);
}
