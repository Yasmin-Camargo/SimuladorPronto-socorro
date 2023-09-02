/* NOMES: CAROLINE SOUZA CAMARGO  <caroline.sc@inf.ufpel.edu.br>
          YASMIN SOUZA CAMARGO    <yasmin.sc@inf.ufpel.edu.br>
          BIANCA BEPPLER DULLIUS  <bianca.bd@inf.ufpel.edu.br>
  
  Este trabalho simula uma ala de pronto-socorro para pacientes com problemas respiratórios, onde cada paciente e médico são representados como threads. A sala tem nebulizadores limitados, e os pacientes disputam seu uso. Um chefe de enfermeiros gerencia o acesso aos nebulizadores. Os pacientes têm sinais vitais que variam, e o tratamento com nebulizadores afeta seus sinais vitais. O tempo de atendimento dos médicos é aleatório, e os pacientes são liberados após o tratamento. O programa principal lança threads de pacientes com tempos aleatórios até que um tempo de simulação seja atingido, garantindo que todas as threads tenham tempo para serem atendidas antes do término.
  */

#include "filaAtendimentoMedico.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

//Parâmetros da simulação que podem ser modificados
#define NUM_MAX_NEBULIZADORES 4
#define TEMPO_MAXIMO_USANDO_NEBULIZADOR 5
#define NUM_MAX_PACIENTES_SALA_ESPERA 12
#define NUM_MEDICOS 2
#define TEMPO_MAXIMO_ENTRE_CHEGADAS_PACIENTES 2
#define NUM_MAX_PACIENTES_EMERGENCIA (NUM_MAX_PACIENTES_SALA_ESPERA + NUM_MAX_NEBULIZADORES)
#define TEMPO_FUNCIONAMENTO_HOSPITAL 35
#define TEMPO_ATENDIMENTO_MEDICO 7
#define TAXA_ATUALIZACAO_SINAL_VITAL 3

//Variáveis Compartilhadas
int quantPacientes = 0, 
    quantPacientesParadaRespiratoria = 0,
    terminoAtendimento = 0,
    pacienteUtilizandoNebulizador[NUM_MAX_NEBULIZADORES], 
    pacientes[NUM_MAX_PACIENTES_EMERGENCIA]; /*CÓDIGOS  -1: não existe paciente  
                                                        -2: local reservado para um paciente
                                                        -3: paciente atendido pelo médico
                                                        0 < x < 10: sinal vital paciente */

//Variáveis de sincronização
sem_t sem_nebulizadores;
pthread_mutex_t mutex_quantPacientes, 
                mutex_quantPacientesParadaRespiratoria,
                mutex_pacientes, 
                mutex_pacienteUtilizandoNebulizador,
                mutex_proxFilaAtendimentoMedico;

//Escopo das Funções
void* thread_paciente_SalaDeEspera(void *arg);
void* thread_paciente_OutroHospital(void *arg);
void* thread_medico(void *arg);
int alocaCadeira();
void triagemEnfermeiro(int posPaciente, int idPaciente);
int enfermeiroChefePrioridadeNebulizadores();
int verificaUtilizacaoNebulizador(int posPaciente);
void usarNebulizador(int posPaciente, int idPaciente);
int getTempoNebulizadorEnfermeiroChefe();
void diminuiSinalVitalPacientes();
int geraValorAleatorio(int max);
void inicializaVetores();


//Implementação
void* thread_paciente_OutroHospital(void *arg) { //Pacientes que não são atendidos não mandados para essa thread
  int id_thread = *(int*)arg;
  printf("\n:( Paciente %d redirecionado para outro hospital\n", id_thread);
  pthread_exit(NULL);
}

void* thread_paciente_SalaDeEspera(void *arg) {
  int id_thread = *(int*)arg;
  int posPaciente = alocaCadeira(); //Aloca um assento para o paciente (pode ser na sala de espera ou no ala dos nebulizadores)

  printf("\n-> Paciente %d ocupando assento %d na sala de espera\n", id_thread, posPaciente);
  fflush(stdout);
  
  triagemEnfermeiro(posPaciente, id_thread); //Inicialmente o paciente passa pelo processo de triagem

  pthread_mutex_lock(&mutex_proxFilaAtendimentoMedico); //Coloca para a fila de atendimento médico
  Push_filaAtendimentoMedico(posPaciente);
  pthread_mutex_unlock(&mutex_proxFilaAtendimentoMedico);

  pthread_mutex_lock(&mutex_pacientes);
  while (pacientes[posPaciente] != -3) { //O paciente fica esperando atendimento médico (cod: -3) e disputando o uso do nebulizador 
    pthread_mutex_unlock(&mutex_pacientes);

    if (enfermeiroChefePrioridadeNebulizadores() == posPaciente) { //O chefe dos enfermeiros diz quem vai ter acesso aos nebulizadores
      int quantAtualNebulizadorDisponivel = -1;
      sem_getvalue(&sem_nebulizadores, &quantAtualNebulizadorDisponivel); //Espia se existe nebulizador vago 

      if (quantAtualNebulizadorDisponivel > 0) { //Caso tenha, fica esperando pelo uso do nebulizador
        sem_wait(&sem_nebulizadores);
        usarNebulizador(posPaciente, id_thread); 
        sem_post(&sem_nebulizadores); 
      }
    } 
    sleep(1);
    pthread_mutex_lock(&mutex_pacientes);
  }
  pacientes[posPaciente] = -1; //Libera assento na sala de espera
  pthread_mutex_unlock(&mutex_pacientes);

  pthread_mutex_lock(&mutex_quantPacientes);
  quantPacientes--; //Diminui quantidade de pessoas na sala de espera
  pthread_mutex_unlock(&mutex_quantPacientes);

  printf("\nPACIENTE %d GANHOU ALTA\n", id_thread);
  fflush(stdout);

  pthread_exit(NULL);
}

int alocaCadeira(){
  for (int i = 0; i < NUM_MAX_PACIENTES_EMERGENCIA; i++) {
    pthread_mutex_lock(&mutex_pacientes);
    if (pacientes[i] == -1) { //Cadeira não ocupada
      pacientes[i] = -2; //Indica que a cadeira foi reservada mas não foi medido o sinal vital
      pthread_mutex_unlock(&mutex_pacientes);
      return i;
    } else{
      pthread_mutex_unlock(&mutex_pacientes);
    }
  }

  printf("\nERRO, nao possui cadeira disponiveis para o paciente\n");
  fflush(stdout);
  return -1;
}

void triagemEnfermeiro(int posPaciente, int idPaciente){ 
  int tempSinalVital = geraValorAleatorio(10); //Enfermeiro mede sinal vital do paciente
  printf("\nTriagem do paciente %d realizada com sucesso, sinal vital: %d\n", idPaciente, tempSinalVital);
  fflush(stdout);
  
  pthread_mutex_lock(&mutex_pacientes);
  pacientes[posPaciente] = tempSinalVital; //Atualiza sinal vital 
  pthread_mutex_unlock(&mutex_pacientes);
  
  if (tempSinalVital == 0) { //Se chegou em zero significa que o paciente está muito mal
    printf("\nPaciente %d sofreu parada respiratória\n", idPaciente);
    fflush(stdout);
    
    pthread_mutex_lock(&mutex_quantPacientesParadaRespiratoria);
    quantPacientesParadaRespiratoria++;
    pthread_mutex_unlock(&mutex_quantPacientesParadaRespiratoria);
  }
}

int enfermeiroChefePrioridadeNebulizadores(){  //Politica de escalonamento utilizada: quem possui o menor sinal vital 
  int indicePacientePrioridade = -1, sinalVitalPacientePrioridade = 9999999; 

  for (int i = 0; i < NUM_MAX_PACIENTES_EMERGENCIA; i++) {
    pthread_mutex_lock(&mutex_pacientes); 
    if (pacientes[i] >= 0 && verificaUtilizacaoNebulizador(i) == 0 && pacientes[i] < sinalVitalPacientePrioridade) { //Precisa estar na sala de espera  e não estar utilizando o nebulizador
      indicePacientePrioridade = i;  
      sinalVitalPacientePrioridade = pacientes[i]; 
    } 
    pthread_mutex_unlock(&mutex_pacientes);
  }

  if (indicePacientePrioridade == -1) {
    printf("\nNão foi localizado um paciente para usar o nebulizador!\n");
    fflush(stdout);
    return 999999;
  }
   
  return indicePacientePrioridade;
}

int verificaUtilizacaoNebulizador(int posPaciente){
  for (int i = 0; i < NUM_MAX_NEBULIZADORES; i++) {
    pthread_mutex_lock(&mutex_pacienteUtilizandoNebulizador);
    if (pacienteUtilizandoNebulizador[i] == posPaciente) {
      pthread_mutex_unlock(&mutex_pacienteUtilizandoNebulizador);
      return 1; //Está usando o nubulizador
    } else {
      pthread_mutex_unlock(&mutex_pacienteUtilizandoNebulizador);
    }
  }
  return 0; //Não está usando o nubulizador
}


void* thread_medico(void *arg) { //O médico atende o paciente mais velho (que está a mais tempo esperando, que chegou primeiro)
  int id_thread = *(int*)arg; 

  pthread_mutex_lock(&mutex_proxFilaAtendimentoMedico);
  int indicePaciente = Pop_filaAtendimentoMedico();
  pthread_mutex_unlock(&mutex_proxFilaAtendimentoMedico);
  
  while (terminoAtendimento == 0) { 
    if (indicePaciente != -1) { //Verifica se existe um paciente para ser atendido
      pthread_mutex_lock(&mutex_pacientes); 
      
      if (pacientes[indicePaciente] != -1) { //Certifica-se se existe alguem relmente no vetor de pacientes
        pacientes[indicePaciente] = -3; //Avisa que o paciente está em atendimento médico e pode liberar a vaga da sala de espera
        pthread_mutex_unlock(&mutex_pacientes);
        
        printf("\n+ Medico %d realizando atendimento\n", id_thread);
        fflush(stdout);

        sleep(TEMPO_ATENDIMENTO_MEDICO); //Atende o paciente

        //Pega o próximo paciente
        pthread_mutex_lock(&mutex_proxFilaAtendimentoMedico);
        indicePaciente = Pop_filaAtendimentoMedico();
        pthread_mutex_unlock(&mutex_proxFilaAtendimentoMedico);

      } else {
        pthread_mutex_unlock(&mutex_pacientes);
        printf("\nErro!, o assento %d esta vazio e nao existe paciente para ser atendido\n", indicePaciente);
        fflush(stdout);
      }
      
    } else { //O médico toma um café e chama o próximo a ser atendido
      sleep(1);
      
      pthread_mutex_lock(&mutex_proxFilaAtendimentoMedico);
      indicePaciente = Pop_filaAtendimentoMedico();
      pthread_mutex_unlock(&mutex_proxFilaAtendimentoMedico);
    }
  }

  pthread_exit(NULL);
}

void usarNebulizador(int posPaciente, int idPaciente){
  int posNebulizador = -1;
  for (int i = 0; i < NUM_MAX_NEBULIZADORES; i++) { //Precisa encontrar o nebulizador que não está sendo usado
    pthread_mutex_lock(&mutex_pacienteUtilizandoNebulizador);
    if (pacienteUtilizandoNebulizador[i] == -1) { //Aloca o nebulizador para o paciente (cod: -1 nebulizador não esta sendo utilizado)
      pacienteUtilizandoNebulizador[i] = posPaciente; //Obs.: O nebulizador guarda a informação do assento do paciente (posição no vetor de pacientes)
      pthread_mutex_unlock(&mutex_pacienteUtilizandoNebulizador);
      posNebulizador = i;
      break;
    } else {
      pthread_mutex_unlock(&mutex_pacienteUtilizandoNebulizador);
    }
  }

  printf("\nPaciente %d conseguiu autorizacao para o uso do nebulizador %d\n", idPaciente, posNebulizador + 1);
  fflush(stdout);
  
  sleep(getTempoNebulizadorEnfermeiroChefe()); //Verifica com o enfermeiro chefe o tempo de utilização do nebulizador

  int novoSinalVital = geraValorAleatorio(2) + 2; //Sinal vital pode aumentar de 2 a 4 niveis
  
  pthread_mutex_lock(&mutex_pacientes);
  if (pacientes[posPaciente] >= 0) {
    pacientes[posPaciente] += novoSinalVital; //Aumenta o sinal vital do paciente
    if (pacientes[posPaciente] > 10){ //Verifica se não passou do limite máximo do sinal vital = 10
      pacientes[posPaciente] = 10;
    }
    printf("\nSinal vital do paciente %d atualizado: %d\n", idPaciente, pacientes[posPaciente]);
    fflush(stdout);
  } 
  pthread_mutex_unlock(&mutex_pacientes);
  
  pthread_mutex_lock(&mutex_pacienteUtilizandoNebulizador);
  pacienteUtilizandoNebulizador[posNebulizador] = -1; //Desaloca o nebulizador que foi utilizado
  pthread_mutex_unlock(&mutex_pacienteUtilizandoNebulizador);
  
  printf("\nNebulizador %d liberado\n", posNebulizador + 1);
  fflush(stdout);
}

int getTempoNebulizadorEnfermeiroChefe(){ 
  return geraValorAleatorio(TEMPO_MAXIMO_USANDO_NEBULIZADOR); //Gera um valor aleatório 
}

int geraValorAleatorio(int max){
  return rand() % (max + 1); //Gera um valor entre 0 e max
}

void diminuiSinalVitalPacientes(){ //Thread que fica de tempo em tempos diminuindo o sinal vital dos pacientes 
  static int cont = 1;
  cont++;
  if (cont > TAXA_ATUALIZACAO_SINAL_VITAL) { //Atualiza sinal vital
    cont = 1;
    for (int i = 0; i < NUM_MAX_PACIENTES_EMERGENCIA; i++) { //Atualiza o sinal vital do pacientes
      pthread_mutex_lock(&mutex_pacientes);
      if (pacientes[i] >= 0 && verificaUtilizacaoNebulizador(i) == 0) { //O paciente não pode estar usando o nebulizador e precisa estar na sala de espera
        int novoSinalVital = pacientes[i] - (geraValorAleatorio(1) + 1); //Sinal vital reduzido em 1 ou 2 níveis
        if (novoSinalVital <= 0) { //Paciente sofreu parada respiratória
          pacientes[i] = 0;
          printf("\nPaciente do assento %d sofreu parada respiratória\n", i);
          fflush(stdout);

          pthread_mutex_lock(&mutex_quantPacientesParadaRespiratoria);
          quantPacientesParadaRespiratoria++;
          pthread_mutex_unlock(&mutex_quantPacientesParadaRespiratoria);

        } else { 
          pacientes[i] = novoSinalVital;
        }    
      }
      pthread_mutex_unlock(&mutex_pacientes);   
    }
  } 
}

void inicializaVetores(){
  for (int i = 0; i < NUM_MAX_NEBULIZADORES; i++) {
    pthread_mutex_lock(&mutex_pacienteUtilizandoNebulizador); 
    pacienteUtilizandoNebulizador[i] = -1;
    pthread_mutex_unlock(&mutex_pacienteUtilizandoNebulizador); 
  }
  for (int i = 0; i < NUM_MAX_PACIENTES_EMERGENCIA; i++) {
    pthread_mutex_lock(&mutex_pacientes); 
    pacientes[i] = -1;
    pthread_mutex_unlock(&mutex_pacientes); 
  }
  Reset_filaAtendimentoMedico();  //Inicializa fila de atendimento médico
}


int main(int argc, char const *argv[]){
  double tempoDecorrido = 0, tempoFuncionamentoHospital = TEMPO_FUNCIONAMENTO_HOSPITAL;
  int totalPessoas = 0, totalPacientesAtendidos = 0, taxaAtualizacaoSinalVital = 4;;

  printf("\n\n\n--------------------------------------------------------------\n");
  printf("\nCONFIGURACOES DE SIMULACAO UTILIZADAS");
  printf("\nNumero de nebulizadores: %d", NUM_MAX_NEBULIZADORES);
  printf("\nNumero de medicos: %d", NUM_MEDICOS);
  printf("\nNumero máximo de pacientes: %d ", NUM_MAX_PACIENTES_EMERGENCIA);
  printf("\nTempo de atendimento medico: 0 - %d ", TEMPO_ATENDIMENTO_MEDICO);
  printf("\nTempo de uso do nebulizador: 0 - %d ", TEMPO_MAXIMO_USANDO_NEBULIZADOR);
  printf("\nTempo de chegada de pacientes no hospital: 0 - %d", TEMPO_MAXIMO_ENTRE_CHEGADAS_PACIENTES);
  printf("\nTempo medio de funcionanmento do hospital: ~ %d", TEMPO_FUNCIONAMENTO_HOSPITAL);

  printf("\n\nCONFIGURACOES DE SIMULACAO FIXAS");
  printf("\nSinal vital do paciente que NAO esta utilizando o nebulizador: 1 - 2 niveis");
  printf("\nSinal vital do paciente que esta utilizando o nebulizador: 2 - 4 níveis\n");
  printf("\nOBS.: Tempo em segundos\n");
  printf("\n------------------------------------------------------------\n");
  fflush(stdout);

  //Inicializa vetores do programa e a semente para a geração de números aleatórios 
  inicializaVetores();
  srand(time(NULL));

  //Inicializando os mutexes e semáforos
  pthread_mutex_init(&mutex_quantPacientes, NULL);
  pthread_mutex_init(&mutex_quantPacientesParadaRespiratoria, NULL);
  pthread_mutex_init(&mutex_pacientes, NULL);
  pthread_mutex_init(&mutex_pacienteUtilizandoNebulizador, NULL);
  pthread_mutex_init(&mutex_proxFilaAtendimentoMedico, NULL);
  sem_init(&sem_nebulizadores, 0, NUM_MAX_NEBULIZADORES);

  //Criando as threads e IDs dos médicos
  pthread_t vet_threadsMedicos[NUM_MEDICOS];
  int ids_Medicos[NUM_MEDICOS];

  //Inicializando os médicos
  for (int i = 0; i < NUM_MEDICOS; i++) {
    ids_Medicos[i] = i + 1;
    pthread_create(&vet_threadsMedicos[i], NULL, thread_medico, &ids_Medicos[i]);

    sleep(1);
  }

  //Criando as threads e IDs das pessoas que chegaram no hospital
  pthread_t *vet_threadsPessoas = NULL;
  vet_threadsPessoas = (pthread_t *) malloc(1 * sizeof(pthread_t)); 
  if (vet_threadsPessoas == NULL) {
      printf("\nErro na alocação de memoria para threads.\n");
      return 1;
  }
  int *ids_Pessoas = NULL;
  ids_Pessoas = (int *) malloc(1 * sizeof(int));
  if (ids_Pessoas == NULL) {
      printf("\nErro na alocação de memoria para threads.\n");
      return 1;
  }

  //Inicializando as pessoas
  for (int i = 0; tempoDecorrido < tempoFuncionamentoHospital; i++) { //A condição de parada é alcançar o tempo de espera estabelecido
    totalPessoas++; //Mais uma pessoa chegou no hospital

    ids_Pessoas = (int *) realloc(ids_Pessoas, totalPessoas * sizeof(int));
    if (ids_Pessoas == NULL) {
      printf("\nErro na alocação de memoria para threads.\n");
      return 1;
    }
    ids_Pessoas[i] = i + 1;

    vet_threadsPessoas = (pthread_t *) realloc(vet_threadsPessoas, totalPessoas * sizeof(pthread_t));
    if (vet_threadsPessoas == NULL) {
      printf("\nErro na alocação de memoria para threads.\n");
      return 1;
    }

    if (quantPacientes < NUM_MAX_PACIENTES_EMERGENCIA) { //Pessoa -> Paciente (Se tem espaço vago na emergência coloca o paciente para para a sala de espera, caso contrário manda ele procurar outro hospital)
      pthread_mutex_lock(&mutex_quantPacientes);
      quantPacientes++;
      pthread_mutex_unlock(&mutex_quantPacientes);
      
      totalPacientesAtendidos++;
     
      pthread_create(&vet_threadsPessoas[i], NULL, thread_paciente_SalaDeEspera, &ids_Pessoas[i]);
      diminuiSinalVitalPacientes(); //Diminui sinal vital dos pacientes
    } else {
      pthread_create(&vet_threadsPessoas[i], NULL, thread_paciente_OutroHospital, &ids_Pessoas[i]);
      fflush(stdout);
    }
    
    // Calcula o tempo até o próximo paciente chegar
    int tempo = geraValorAleatorio(TEMPO_MAXIMO_ENTRE_CHEGADAS_PACIENTES);
    sleep(tempo);
    tempoDecorrido += tempo;
    printf("\nTempo total decorrido: %.2f segundos\n", tempoDecorrido);
    fflush(stdout);
  }

  printf("\n\n====== HOSPITAL FECHADO ======\n\n");

  // Aguardando o término das threads dos PACIENTE
  for (int i = 0; i < totalPessoas; i++) {
    pthread_join(vet_threadsPessoas[i], NULL);
    diminuiSinalVitalPacientes(); //Apesar de ter chegado o fim do atendimento precisa diminuir o sinal vital dos pacientes que ainda estão esperando atendimento
  }

  terminoAtendimento = 1;

  //Aguardando o término das threads dos medicos
  for (int i = 0; i < NUM_MEDICOS; i++) {
    pthread_join(vet_threadsMedicos[i], NULL);
  }

  printf("\n\n\n------------------------- RELATORIO -------------------------\n");
  printf("\nTOTAL DE PACIENTES QUE VISITARAM O HOSPITAL: %d", totalPessoas); 
  printf("\nQUANTIDADE QUE CONSEGUIRAM FICHA PARA CONSULTA: %d", totalPacientesAtendidos);
  printf("\nQUANTOS PACIENTES SOFRERAM PARADA RESPIRATORIA: %d\n", quantPacientesParadaRespiratoria);
  
  printf("\n\nCONFIGURACOES DE SIMULACAO UTILIZADAS");
  printf("\nNumero de nebulizadores: %d", NUM_MAX_NEBULIZADORES);
  printf("\nNumero de medicos: %d", NUM_MEDICOS);
  printf("\nNumero máximo de pacientes: %d ", NUM_MAX_PACIENTES_EMERGENCIA);
  printf("\nTempo de atendimento medico: 0 - %d ", TEMPO_ATENDIMENTO_MEDICO);
  printf("\nTempo de uso do nebulizador: 0 - %d ", TEMPO_MAXIMO_USANDO_NEBULIZADOR);
  printf("\nTempo de chegada de pacientes no hospital: 0 - %d", TEMPO_MAXIMO_ENTRE_CHEGADAS_PACIENTES);
  printf("\nTempo medio de funcionanmento do hospital: ~ %d", TEMPO_FUNCIONAMENTO_HOSPITAL);

  printf("\n\nCONFIGURACOES DE SIMULACAO FIXAS");
  printf("\nSinal vital do paciente que NAO esta utilizando o nebulizador: 1 - 2 niveis");
  printf("\nSinal vital do paciente que esta utilizando o nebulizador: 2 - 4 níveis\n");
  printf("\nOBS.: Tempo em segundos\n");
  printf("\n------------------------------------------------------------\n");
  fflush(stdout);

  //Destruindo mutexes e semáforos
  pthread_mutex_destroy(&mutex_quantPacientes);
  pthread_mutex_destroy(&mutex_quantPacientesParadaRespiratoria);
  pthread_mutex_destroy(&mutex_pacientes);
  pthread_mutex_destroy(&mutex_pacienteUtilizandoNebulizador);
  pthread_mutex_destroy(&mutex_proxFilaAtendimentoMedico);
  sem_destroy(&sem_nebulizadores);

  //Liberando memória
  free(vet_threadsPessoas);
  free(ids_Pessoas);
  Clear_filaAtendimentoMedico();

  return 0;
}