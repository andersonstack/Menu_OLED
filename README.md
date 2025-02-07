# Projeto de Controle de Joystick com Display OLED e PWM

Este projeto utiliza um joystick, LEDs RGB e um display OLED para controlar e visualizar interações. Ele foi desenvolvido utilizando a plataforma RP2040 (Raspberry Pi Pico), com o objetivo de manipular a intensidade dos LEDs e a execução de uma melodia, além de permitir interação com o joystick para navegar em um menu e controlar a música.

## Funcionalidades

- **Joystick**:
  - Leitura dos valores dos eixos X e Y para navegação.
  - Leitura do botão do joystick para selecionar opções.
- **Display OLED**:

  - Exibe imagens bitmap relacionadas à opção selecionada no menu.
  - Atualiza a tela conforme a interação do usuário com o joystick.

- **PWM (Pulse Width Modulation)**:

  - Controla a intensidade de LEDs RGB.
  - Um LED é controlado pelo joystick, alterando sua cor dependendo dos valores dos eixos.
  - O buzzer toca uma melodia em resposta a um evento.

- **Menu Interativo**:
  - O usuário pode navegar por um menu utilizando o joystick, com as opções sendo alteradas à medida que o joystick é movido.
  - O botão do joystick serve para selecionar uma opção do menu e executar ações associadas.

## Componentes

- **Raspberry Pi Pico (RP2040)**.
- **Joystick** com 2 eixos (X, Y) e um botão.
- **Display OLED SSD1306** (I2C).
- **LEDs RGB** (controlados via PWM).
- **Buzzer** para reprodução de notas musicais.

## Como Funciona

### Inicialização

- O código começa inicializando a comunicação I2C com o display OLED.
- O joystick é configurado para ler os eixos analógicos e o botão.

### Interação com o Joystick

- O joystick lê os valores dos eixos X e Y para movimentação no menu.
- O botão do joystick (quando pressionado) ativa a seleção da opção do menu.

### Navegação no Menu

- O menu possui três opções:
  - **Opção 1**: Exibe o bitmap relacionado à seleção do joystick.
  - **Opção 2**: Exibe o bitmap do buzzer.
  - **Opção 3**: Exibe o bitmap do LED.
- O usuário pode alterar a opção selecionada movendo o joystick para a esquerda ou para a direita. O botão do joystick seleciona a opção.

### Controle do LED

- A intensidade do LED pode ser ajustada com o joystick, alterando sua cor.

### Reprodução de Música

- O buzzer toca uma melodia utilizando PWM e a música é reproduzida enquanto o joystick não for pressionado.

## Dependências

- **Bibliotecas para o RP2040**:
  - `hardware/i2c.h` para comunicação I2C com o display.
  - `hardware/pwm.h` para controle de PWM nos LEDs.
  - `hardware/adc.h` para ler os valores analógicos do joystick.
  - `ssd1306_i2c.h` para controle do display OLED.

## Como Compilar e Rodar

1. Instale as dependências necessárias para o ambiente RP2040.
2. Compile o código com o compilador apropriado para o Raspberry Pi Pico.
3. Carregue o arquivo binário no dispositivo.
4. Conecte o display OLED, joystick, LEDs e buzzer ao Raspberry Pi Pico conforme as configurações no código.
5. Caso de erro:

```
rm -rf build && cd build && cmake -G Ninja ..
```

## Licença

Este projeto está sob a licença MIT. Veja o arquivo [LICENSE](LICENSE) para mais detalhes.
