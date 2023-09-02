# Simulador de Pronto-Socorro Respiratório

Este projeto de Sistemas Operacionais foi desenvolvido em colaboração com @Yasmin-Camargo e @BiancaBDullius e simula uma ala de pronto-socorro dedicada ao tratamento de pacientes com problemas respiratórios. Nele, cada paciente e médico são representados como threads.


## Funcionalidades Principais

- **Recursos Limitados**: A sala de atendimento possui um número limitado de nebulizadores, e os pacientes competem pelo seu uso.

- **Politica de Escalonamento dos Nebulizadores**: Um chefe de enfermeiros supervisiona o acesso aos nebulizadores, garantindo que os pacientes sejam atendidos de maneira adequada.

- **Sinais Vitais Dinâmicos**: Os pacientes têm sinais vitais que variam de 0 a 10, e o tratamento com nebulizadores afeta esses sinais vitais.

- **Atendimento Aleatório**: O tempo de atendimento dos médicos e de uso dos nebulizadores é aleatório, tornando a simulação mais realista.

## Configurações Personalizáveis

Este simulador permite a personalização de várias configurações para ajustar a simulação de acordo com as necessidades:

- Número de nebulizadores
- Número de médicos
- Número máximo de pacientes
- Tempo de atendimento médico
- Tempo de uso do nebulizador
- Taxa de atualização do sinal vital
- Variação do tempo de chegada de pacientes no pronto-socorro
- Variação do tempo médio de funcionamento do pronto-socorro

Dessa forma, é possível explorar diferentes cenários e avaliar o funcionamento do pronto-socorro em várias situações. Vale ressaltar, que este trabalho foi desenvolvido exclusivamente para fins educacionais, permitindo a compreensão de conceitos relacionados a sistemas operacionais e escalonamento de recursos.

## Como Usar

Basta compilar e executar o programa para iniciar a simulação do pronto-socorro respiratório.

```bash
gcc main.c filaAtendimentoMedico.c -o main -lpthread
.\main