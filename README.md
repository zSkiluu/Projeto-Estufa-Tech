# Projeto-Estufa-Tech

A Estufa Tech tem como objetivo integrar hardware e software por meio da comunicação entre Arduino e Elipse SCADA, formando um sistema inteligente e autônomo de automação para cultivo.
O sistema monitora as principais variáveis de processo (PV):
Umidade do solo


Intensidade da luz recebida pela planta


Para cada uma dessas variáveis, o operador pode definir um setpoint (SP) ideal diretamente pela interface SCADA, ou seja, o nível desejado de umidade e luminosidade.
Com base na comparação entre os valores medidos (PV) e os valores definidos (SP), o sistema aciona automaticamente os atuadores:
Um motor de microondas adaptado controla a entrada de luz (abrindo ou fechando uma cobertura, por exemplo);


Uma bomba d'água é utilizada para realizar a irrigação do solo.


Esse controle em malha fechada permite que a estufa mantenha condições ideais para o cultivo de forma precisa, remota e automatizada, otimizando recursos e garantindo melhores resultados.

Além disso nesta imagem exemplifico o software Elipse SCADA como supervisório para controle de maneira serial do projeto

![image](https://github.com/user-attachments/assets/67365f88-b9b5-491f-b9f7-2d6d49c70fcd)
