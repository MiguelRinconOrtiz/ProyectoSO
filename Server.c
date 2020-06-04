#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <mysql.h>
#include <pthread.h>
#include <ctype.h>

typedef struct {
	char nombre [20];
	int socket;
} Conectado;

typedef struct {
	Conectado conectados [100];
	int num;
} ListaConectados;

typedef struct {
	Conectado jugador [4]; //invitados a partida
	Conectado creador; //creador de partida
	int id; //identifiador partida
	int numj; //numero de jugadores invitados
	int accept; //para comprobar si todos han aceptado
} Partida;

typedef struct {
	Partida partida [100];
	int nump;
} ListaPartidas;

ListaConectados miLista;
ListaPartidas miTabla;

MYSQL *conn;

char peticion[512];
char respuesta[512];
int count, ret;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//ACCESO EXLUYENTE

int DamePos(int socket){
	int i=0;
	while (i<miLista.num){
		if (miLista.conectados[i].socket ==socket)
			return i;
		i++;
	}
	return -1;
}
int DameSock(char nombre[200]){
	int i=0;
	while (i<miLista.num){
		if (strcmp(miLista.conectados[i].nombre, nombre) ==0)
			return miLista.conectados[i].socket;
		i++;
	}
	return -1;
}
int addNameLista (char nombre[20], int socket){
	int pos =DamePos(socket);
	if (pos==-1)
		return -1;
	else{
		pthread_mutex_lock(&mutex); 
		strcpy(miLista.conectados[pos].nombre, nombre);
		pthread_mutex_unlock(&mutex);
		return 0;
	}
}

int delLista (int socket){
	int pos = DamePos(socket);
	if (pos == -1)
		return -1;
	pthread_mutex_lock(&mutex);
	for(int i=pos;i<miLista.num-1;i++){
		strcpy(miLista.conectados[i].nombre, miLista.conectados[i+1].nombre);
		miLista.conectados[i].socket = miLista.conectados[i+1].socket;
	}
	miLista.num--;
	pthread_mutex_unlock(&mutex); 
	return 0;
}
void DameLista (char respuesta[512]){
	int i=0;
	if (miLista.num == 0)
		sprintf (respuesta, "-1");
	else {
		for (int i=0;i<miLista.num;i++)
			sprintf (respuesta, "%s%s\n ", respuesta, miLista.conectados[i].nombre);
	}
}
void DameListaSockets (char respuesta[512]){
	int i=0;
	if (miLista.num == 0)
		sprintf (respuesta, "-1");
	else {
		for (int i=0;i<miLista.num;i++)
			sprintf (respuesta, "%s%d, ", respuesta, miLista.conectados[i].socket);
	}
}
int signIN(char nombre[20], char passw[20], int edad){
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[512];
	int err = mysql_query(conn, "SELECT username FROM Jugadores");
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	resultado = mysql_store_result(conn); 
	row = mysql_fetch_row(resultado);
	int id = 1;
	int found = 0;
	while (row != NULL){
		for(int i =0;i<strlen(nombre);i++)
			nombre[i] =toupper(nombre[i]);
		for(int i =0;i<strlen(row[0]);i++)
			row[0][i]= toupper(row[0][i]);
			if (strcmp(nombre, row[0]) == 0) //si coinsideix el nom amb user existent
				found = 1;
			id++; //incrementa contador
			row = mysql_fetch_row(resultado);
	}
	if (!found){//en cas que user no es troba a BBDD l'afegim
		sprintf(consulta,"INSERT INTO Jugadores VALUES (%d,'%s','%s', %d, 0);",id,nombre,passw,edad);
		puts(consulta);	
		err = mysql_query(conn, consulta);
		if (err!=0) {
			printf ("Error al introducir datos la base %u %s\n", mysql_errno(conn), mysql_error(conn));
			exit (1);
		}
		return id;
	}else 
		return -1;
}
int logIN(char nombre[20], char passw[20]){
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[512];
	sprintf (consulta,"SELECT psswrd FROM Jugadores WHERE username = '%s'", nombre); 
	int err=mysql_query (conn, consulta); 
	if (err!=0) {
		sprintf (respuesta, "Error al consultar datos de la base %u %s\n",mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	resultado = mysql_store_result (conn); 
	row = mysql_fetch_row (resultado);
	if (row == NULL)
		return -2; // no existe user 
	else{
		if(strcmp(row[0],passw)!=0){ //no coinsideix contrasenya
			return -1;
		}else
		   return 0; 
	}
}
void HacerConsulta(char consulta[512], char respuesta[200]){
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	int err=mysql_query (conn, consulta); 
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn), mysql_error(conn));
		strcpy (respuesta, "-2"); // error en consulta
		exit (1);
	}
	resultado = mysql_store_result (conn); 
	row = mysql_fetch_row (resultado);
	if (row == NULL)
		strcpy (respuesta, "-1"); //no hay resultados 
	else{
		strcpy (respuesta, " ");
		while(row!=NULL){
			sprintf(respuesta, "%s%s, ", respuesta, row[0]);
			row = mysql_fetch_row (resultado);
		}
	}
}

void Consulta1(int id, char respuesta[200]){
	char consulta[512];
	sprintf (consulta,"SELECT Jugadores.username FROM Jugadores,Participacion WHERE Participacion.Partida = %d ", id);
	strcat(consulta, "and Participacion.Jugador = Jugadores.Id and Jugadores.age<18;"); 
	HacerConsulta(consulta, respuesta);
}
void Consulta2(char nombre[20], char respuesta[200]){
	char consulta[512];
	sprintf (consulta,"SELECT Partidas.Ciudad FROM Jugadores,Participacion,Partidas WHERE Jugadores.username = '%s' ", nombre);
	strcat(consulta, "and Participacion.Jugador = Jugadores.Id and Partidas.Id = Participacion.Partida;"); 
	HacerConsulta(consulta, respuesta);
}
void Consulta3(char nombre[20], char respuesta[200]){
	char consulta[512];
	sprintf (consulta,"SELECT Jugadores.username FROM Jugadores,Participacion,Partidas WHERE Partidas.winner = '%s' and", nombre);
	strcat (consulta, " Participacion.Partida = Partidas.Id and Jugadores.Id = Participacion.Jugador and Jugadores.username!='");
	sprintf (consulta, "%s%s';",consulta, nombre);
	HacerConsulta(consulta, respuesta);
}

void *AtenderCliente(void *socket){
	int sock_conn;
	int *s;
	s=(int *) socket;
	sock_conn = *s;
	
	//Creamos una conexion al servidor MYSQL 
	conn = mysql_init(NULL);
	if (conn==NULL) {
		printf ("Error al crear la conexion: %u %s\n", mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	conn = mysql_real_connect (conn, "shiva2.upc.es","root", "mysql","T8_BBDDJuego", 0, NULL, 0);
	if (conn==NULL)
	{
		printf ("Error al inicializar la conexion: %u %s\n", mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	int terminar =0;
	// Entramos en un bucle para atender todas las peticiones de este cliente hasta que se desconecte
	while (terminar ==0){
		char nombre[20];
		char passw[20];
		int edad, id;
		char conectado[20];
		// Ahora recibimos la petici?n
		ret=read(sock_conn,peticion, sizeof(peticion));
		printf ("Recibido\n");
		
		// Tenemos que a?adirle la marca de fin de string 
		// para que no escriba lo que hay despues en el buffer
		peticion[ret]='\0';
		printf ("Peticion: %s\n",peticion);
		
		char *p = strtok( peticion, "/");
		int codigo =  atoi (p); //obtenemos codigo del servicio
		printf("codigo %d\n",codigo);
		if (codigo ==0){ //petici?n de desconexi?n
			terminar=1;
			int del = delLista(sock_conn);
			if (del == -1)
				printf("error en desconexion\n");
			else{
				printf("%s se ha desconectado\n", conectado);
				char noms[200];
				strcpy(noms, "6/");//codiigo lista conectados
				DameLista (noms);
				for(int j=0;j<miLista.num;j++)
					write (miLista.conectados[j].socket,noms, strlen(noms));
			}
		}
		else if(codigo ==1){ //darse de alta
			p = strtok( NULL, "/");
			strcpy (nombre, p); 
			p = strtok( NULL, "/");
			strcpy(passw,p);
			p = strtok( NULL, "/");
			edad = atoi(p);
			//Miramos cuantos usuarios hay en la BBDD
			id = signIN(nombre, passw, edad);
			if (id == -1) //error
				sprintf(respuesta, "1/-1");
			else 
				sprintf (respuesta,"1/0/%d/%s",id,nombre);
		}
		else if (codigo ==2){ //iniciar sesion
			p = strtok( NULL, "/");
			strcpy (conectado, p);
			p = strtok( NULL, "/");
			strcpy(passw, p);
			int res = logIN(conectado, passw);
			if(res == 0){
				int add = addNameLista (conectado, sock_conn);
				if (add ==0){
					sprintf (respuesta, "2/0");
					printf("Anadid@. Socket de %s es %d\n", conectado, sock_conn);
					char noms[200];
					strcpy(noms, "6/");//codiigo lista conectados
					DameLista (noms);
					for(int j=0;j<miLista.num;j++)
						write (miLista.conectados[j].socket,noms, strlen(noms));
				}
			}
			else if(res == -1)
			   sprintf (respuesta, "2/-2"); //contra erronia
			else
				sprintf (respuesta, "2/-3"); //no existe user
		}
		else if (codigo == 3){ //consulta 1
			p = strtok(NULL, "/");
			id =  atoi (p);
			char noms[200];
			Consulta1(id, noms);
			sprintf(respuesta,"3/%s", noms);
		}
		else if (codigo == 4){ //consulta 2
			p = strtok(NULL, "/");
			strcpy(nombre, p);
			char noms[200];
			Consulta2(nombre, noms);
			sprintf(respuesta,"4/%s", noms);
		}
		else if (codigo == 5){ //consulta 3
			p = strtok(NULL, "/");
			strcpy(nombre, p);
			char noms[200];
			Consulta3(nombre, noms);
			sprintf(respuesta,"5/%s", noms);
		}
		else if (codigo == 8){ //lista de sockets
			char noms[200];
			strcpy(noms, "8/");
			DameListaSockets (noms);
			strcpy(respuesta, noms);
		}
		else if (codigo==9){ //invitacion
			miTabla.partida[miTabla.nump].id = miTabla.nump;
			miTabla.partida[miTabla.nump].creador= miLista.conectados[DamePos(sock_conn)];
			miTabla.partida[miTabla.nump].accept =0;
			p = strtok(NULL, "/");
			int num =  atoi (p);//numero invitados
			miTabla.partida[miTabla.nump].numj=num;
			char invitado[20];
			sprintf (respuesta, "9/%d/%s", miTabla.nump, conectado);
			for(int j =0; j<num;j++){
				p = strtok( NULL, ",");
				strcpy(invitado,p);
				miTabla.partida[miTabla.nump].jugador[j] = miLista.conectados[DamePos(DameSock(invitado))];
				write(DameSock(invitado), respuesta, strlen(respuesta));
			}
			miTabla.nump++;
		}
		else if (codigo == 10){ //aceptar invitacion
			p = strtok(NULL, "/");
			id =  atoi (p);//numero partida
			p = strtok(NULL, "/");
			int respons =  atoi (p);
			if (respons == 1){//aquesta persona no ha acceptat
				sprintf(respuesta,"10/%d/-1", id);//
				for(int j =0; j<miTabla.partida[id].numj;write(miTabla.partida[id].jugador[j].socket, respuesta, strlen(respuesta)), j++);
				write(miTabla.partida[id].creador.socket, respuesta, strlen(respuesta));
			}
			else
				miTabla.partida[id].accept = miTabla.partida[id].accept+1; //aumentem en nombre de respostes
			if (miTabla.partida[id].accept==miTabla.partida[id].numj){ //si tothom ha respos si
				sprintf(respuesta,"10/%d/0", id);//tothom ha acceptat
				for(int j =0; j<miTabla.partida[id].numj;write(miTabla.partida[id].jugador[j].socket, respuesta, strlen(respuesta)), j++);
				write(miTabla.partida[id].creador.socket, respuesta, strlen(respuesta));
			}
		}
		else if (codigo ==11){//chat
			p = strtok(NULL, "/");
			id =  atoi (p);//numero partida
			p = strtok(NULL, "/");
			char mensaje[200];
			strcpy(mensaje, p);
			sprintf(respuesta,"11/%s: %s", conectado, mensaje);//tothom ha acceptat
			for(int j =0; j<miTabla.partida[id].numj;write(miTabla.partida[id].jugador[j].socket, respuesta, strlen(respuesta)), j++);
			write(miTabla.partida[id].creador.socket, respuesta, strlen(respuesta));
		}
		else
				 printf("no hay consulta");
		
		if ((codigo==1)||(codigo==2)||(codigo==3)||(codigo==4)||(codigo==5)||(codigo==8)){ // enviamos la respuesta al cliente
			printf ("Respuesta: %s\n", respuesta);
			write (sock_conn,respuesta, strlen(respuesta));
		}
		if ((codigo!=0)&&(codigo!=7)&&(codigo!=8)&&(codigo!=6)){
			pthread_mutex_lock(&mutex); //no interrumpir
			count++;//modificacion estructura compartida
			pthread_mutex_unlock(&mutex); //se puede interrumpir
			char noms[200];
			sprintf(noms, "7/%d", count);//codigo numero de peticiones
			for(int j=0;j<miLista.num;j++)
				write (miLista.conectados[j].socket,noms, strlen(noms));
		}
	}
	close(sock_conn); // Se acabo el servicio para este cliente
	if (miLista.num == 0)
		mysql_close (conn);
}

int main(int argc, char *argv[]){ 
	miLista.num =0;
	miTabla.nump=0;
	struct sockaddr_in serv_adr;
	int sock_conn, sock_listen;
	int puerto = 50074;
	// INICIALITZACIONS
	// Obrim el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Error creant socket\n");
	// Fem el bind al port
	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr
	serv_adr.sin_family = AF_INET;
	
	// asocia el socket a cualquiera de las IP de la m?quina. 
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(puerto);// establecemos el puerto de escucha
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		printf ("Error al bind\n");
	
	if (listen(sock_listen, 20) < 0) //
		printf("Error en el Listen\n");
	count =0;
	int i=0;
	pthread_t thread[100];
	for (;;){	// 
		printf ("Escuchando\n");
		sock_conn = accept(sock_listen, NULL, NULL);//socket que usaremos para este cliente
		printf ("He recibido conexion sock %d\n", sock_conn);
		if (miLista.num==100)
			printf("lista llena sock\n");
		else{
			miLista.conectados[miLista.num].socket=sock_conn;
			miLista.num++;
			pthread_create (&thread[i], NULL, AtenderCliente, &miLista.conectados[miLista.num-1].socket);
			i++;
		}
	}
	exit(0);
}
