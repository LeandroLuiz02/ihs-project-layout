# Guia: Usando Driver Python como Alternativa

Se você encontrar problemas com o driver C e precisar usar a versão Python, aqui está como adaptar:

## Por que usar Python?
- **Debugging mais fácil**: Erros são mais claros
- **Desenvolvimento rápido**: Não precisa recompilar
- **Prototipagem**: Testar ideias rapidamente

## Como Adaptar o Projeto

### 1. Verificar Exemplo Python Existente
```bash
cat exemples/python/app-pci.py
```

### 2. Criar Wrapper Python para o Jogo
Crie um arquivo `python_hardware.py`:

```python
#!/usr/bin/env python3
import os
import struct
import fcntl

# IOCTL commands (mesmos do C)
RD_SWITCHES   = 0x6161  # _IO('a', 'a')
RD_PBUTTONS   = 0x6162  # _IO('a', 'b')  
WR_L_DISPLAY  = 0x6163  # _IO('a', 'c')
WR_R_DISPLAY  = 0x6164  # _IO('a', 'd')
WR_RED_LEDS   = 0x6165  # _IO('a', 'e')
WR_GREEN_LEDS = 0x6166  # _IO('a', 'f')

class FPGAController:
    def __init__(self, device_path="/dev/de2i-150"):
        try:
            self.fd = os.open(device_path, os.O_RDWR)
            print(f"FPGA device opened: {device_path}")
        except OSError as e:
            print(f"Failed to open FPGA device: {e}")
            self.fd = None
    
    def write_leds(self, red_pattern, green_pattern):
        if self.fd is None:
            return
            
        # Red LEDs
        fcntl.ioctl(self.fd, WR_RED_LEDS)
        os.write(self.fd, struct.pack('I', red_pattern))
        
        # Green LEDs  
        fcntl.ioctl(self.fd, WR_GREEN_LEDS)
        os.write(self.fd, struct.pack('I', green_pattern))
    
    def write_displays(self, left_value, right_value):
        if self.fd is None:
            return
            
        # Left display
        fcntl.ioctl(self.fd, WR_L_DISPLAY)
        os.write(self.fd, struct.pack('I', left_value))
        
        # Right display
        fcntl.ioctl(self.fd, WR_R_DISPLAY) 
        os.write(self.fd, struct.pack('I', right_value))
    
    def read_inputs(self):
        if self.fd is None:
            return 0, 0
            
        # Read switches
        fcntl.ioctl(self.fd, RD_SWITCHES)
        switches_data = os.read(self.fd, 4)
        switches = struct.unpack('I', switches_data)[0]
        
        # Read buttons
        fcntl.ioctl(self.fd, RD_PBUTTONS)
        buttons_data = os.read(self.fd, 4)
        buttons = struct.unpack('I', buttons_data)[0]
        
        return switches, buttons
    
    def close(self):
        if self.fd is not None:
            os.close(self.fd)
```

### 3. Integrar com o Jogo C++
Você pode usar Python como bridge:

```cpp
// No hardware.cpp, substituir chamadas diretas por:
int system("python3 python_hardware.py update_leds 0xFF00FF00 0x00FF00FF");
```

### 4. Ou Criar Jogo Completo em Python
Use pygame em vez de SDL2:

```bash
pip3 install pygame
```

## Vantagens da Abordagem Python
- **Flexibilidade**: Fácil de modificar em tempo real
- **Debugging**: Print statements simples
- **Rapidez**: Sem recompilação
- **Portabilidade**: Mesmo código em qualquer Linux

## Quando Usar Cada Abordagem

### Use C++ quando:
- Performance é crítica
- Integração total com SDL2
- Produto final/produção

### Use Python quando:  
- Prototipagem rápida
- Debugging de drivers
- Desenvolvimento iterativo
- Problemas com compilação C++

## Comando para Testar Driver Python
```bash
# Testar se driver funciona
python3 exemples/python/app-pci.py /dev/de2i-150

# Se funcionar, pode adaptar todo o projeto
```

Este guia permite que você continue o desenvolvimento mesmo se houver problemas específicos com a compilação C++ na sua placa FPGA.
