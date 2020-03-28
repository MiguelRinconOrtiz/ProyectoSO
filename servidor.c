#include <string.h>

#include <unistd.h>

#include <stdlib.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <stdio.h>

#include <mysql.h>


void insertarJugador(int i, char nombre[20],char passw[20],char res[200])
{
	char consulta[200];
	strcpy (consulta, "INSERT INTO jugador VALUES (");
	sprintf(res,"%s%d,'%s','%s');",consulta,i,nombre,passw);
	printf("Nombre : %s , Passw: %s , id: %d\nConsulta: %s\n",nombre,passw,i,res);
	
	
	
}

int main(int argc, char *argv[])

{

	

	int sock_conn, sock_listen, ret;
	MYSQL_RES *resultado;
	MYSQL_ROW row;

	struct sockaddr_in serv_adr;

	char peticion[512];

	char respuesta[512];
	
	MYSQL *conn;
	int err;
	//Creamos una conexion al servidor MYSQL 
	conn = mysql_init(NULL);
	if (conn==NULL) {
		printf ("Error al crear la conexion: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	conn = mysql_real_connect (conn, "localhost","root", "mysql","juego", 0, NULL, 0);
	if (conn==NULL)
	{
		printf ("Error al inicializar la conexion: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	

	// INICIALITZACIONS

	// Obrim el socket

	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)

		printf("Error creant socket");

	// Fem el bind al port

	

	

	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr

	serv_adr.sin_family = AF_INET;

	

	// asocia el socket a cualquiera de las IP de la m?quina. 

	//htonl formatea el numero que recibe al formato necesario

	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);

	// establecemos el puerto de escucha

	serv_adr.sin_port = htons(9000);

	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)

		printf ("Error al bind");

	

	if (listen(sock_listen, 3) < 0)

		printf("Error en el Listen");

	

	int i=1;

	// Bucle infinito

	for (;;){

		printf ("Escuchando\n");

		

		sock_conn = accept(sock_listen, NULL, NULL);

		printf ("He recibido conexion\n");

		//sock_conn es el socket que usaremos para este cliente

		

		int terminar =0;

		// Entramos en un bucle para atender todas las peticiones de este cliente

		//hasta que se desconecte
		

		while (terminar ==0)

		{
			
			char nombre[20];
			char passw[20];
			char consulta[200];
			char pet[200];
			int id;

			// Ahora recibimos la petici?n

			ret=read(sock_conn,peticion, sizeof(peticion));

			printf ("Recibido\n");

			

			// Tenemos que a?adirle la marca de fin de string 

			// para que no escriba lo que hay despues en el buffer

			peticion[ret]='\0';

		

			printf ("Peticion: %s\n",peticion);

			

			// vamos a ver que quieren

			char *p = strtok( peticion, "/");
			

			int codigo =  atoi (p);

			// Ya tenemos el c?digo de la petici?n

			

			if (codigo ==0) //petici?n de desconexi?n
				terminar=1;
		
			

			else if(codigo ==1)
			{	//peticion de darse de alta
				
				p = strtok( NULL, "/");
				strcpy (nombre, p); 
				p = strtok( NULL, "/");
				strcpy(passw,p);
				//Miramos cuantos usuarios hay en la BBDD
				strcpy(consulta,"SELECT jugador.username FROM jugador;");
				err = mysql_query(conn, consulta);
				if (err!=0) {
					printf ("Error al consultar datos de la base %u %s\n",
							mysql_errno(conn), mysql_error(conn));
					exit (1);
				}
				
				resultado = mysql_store_result(conn); 
				row = mysql_fetch_row(resultado);
				
				if (row == NULL)// Si no hay nadie, inserto el nuevo usuario con id=1
				{ 
				int i=0;
					insertarJugador(1,nombre,passw,pet);
					err = mysql_query(conn, pet);
						if (err!=0) {
							printf ("Error al introducir datos la base %u %s\n", 
								mysql_errno(conn), mysql_error(conn));
							exit (1);
						}
						sprintf (respuesta,"Bienvenido %s, estas dado de alta con id: %d\n",nombre,i+1);
				}
				else{ //Si ya hay alguien, cuento cuantos usuarios hay ,compruebo que el username no coincida e inserto el nuevo usuario con id= numero de usuarios ya existentes + 1
					i=0;
					int encontrado=0;
					while(encontrado==0 && row!=NULL)
					{
						if(strcmp(row[0],nombre)==0)
							encontrado=1;
						else{
							i++;
							row = mysql_fetch_row(resultado);
						}
					}
					
						if(encontrado==1)
						  sprintf (respuesta,"El usurario %s ya existe, prueba con otro nombre\n",nombre);
						else{
					insertarJugador(i+1,nombre,passw,pet);
					err = mysql_query(conn, pet);
						if (err!=0) {
							printf ("Error al introducir datos la base %u %s\n", 
							mysql_errno(conn), mysql_error(conn));
							exit (1);
						}
					sprintf (respuesta,"Bienvenido %s, estas dado de alta con id: %d\n",nombre,i+1);
						}
				}

				
			}

			else if (codigo ==2)

			{// quiere loguearse
				p = strtok( NULL, "/");
				strcpy (nombre, p); //
				p = strtok( NULL, "/");
				strcpy(passw,p);
				strcpy (consulta,"SELECT jugador.password FROM jugador WHERE jugador.username = '"); 
				sprintf(consulta,"%s%s';",consulta,nombre);
				// hacemos la consulta 
				err=mysql_query (conn, consulta); 
				if (err!=0) {
					printf ("Error al consultar datos de la base %u %s\n",
							mysql_errno(conn), mysql_error(conn));
					exit (1);
				}
				
				resultado = mysql_store_result (conn); 
				row = mysql_fetch_row (resultado);
				if (row == NULL)
					 strcpy(respuesta,"No te has registrado\n");
				else
				{//comprobamos si la contraseña asociada a ese usuario es igual al introducido por el cliente
					if(strcmp(row[0],passw)==0)
						strcpy (respuesta,"Bienvenido!\n");
					else

						strcpy (respuesta,"id o contraseña erronia\n");
				}
			}

			else //quiere saber si es alto

			{

				p = strtok( NULL, "/");

				float altura =  atof (p);

				if (altura > 1.70)

					sprintf (respuesta, "%s: eres alto",nombre);

				else

					sprintf (respuesta, "%s: eresbajo",nombre);

			}

				

			if (codigo !=0)

			{

				

				printf ("Respuesta: %s\n", respuesta);

				// Enviamos respuesta

				write (sock_conn,respuesta, strlen(respuesta));

			}

		}
		mysql_close (conn);
		exit(0);

		// Se acabo el servicio para este cliente

		close(sock_conn); 

	}

}

