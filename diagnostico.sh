# Criar teste diagnóstico
cat > test_driver.c << 'EOF'
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#define WR_RED_LEDS   _IO('a', 'e')

int main() {
    printf("=== DIAGNÓSTICO DRIVER ===\n");
    
    int fd = open("/dev/de2i-150", O_RDWR);
    printf("open() resultado: %d\n", fd);
    
    if (fd < 0) {
        printf("ERRO: %s\n", strerror(errno));
        return 1;
    }
    
    printf("✓ Device aberto com sucesso!\n");
    printf("Testando IOCTL WR_RED_LEDS (0x%X)...\n", WR_RED_LEDS);
    
    int ioctl_ret = ioctl(fd, WR_RED_LEDS);
    printf("ioctl() resultado: %d\n", ioctl_ret);
    
    if (ioctl_ret < 0) {
        printf("❌ IOCTL FALHOU: %s (errno=%d)\n", strerror(errno), errno);
        printf("Possíveis causas:\n");
        printf("- Driver não implementou IOCTL corretamente\n");
        printf("- Comando IOCTL inválido\n");
        printf("- Driver não registrou character device\n");
    } else {
        printf("✓ IOCTL funcionou!\n");
        
        uint32_t pattern = 0xF0F0F0F0;
        ssize_t write_ret = write(fd, &pattern, sizeof(pattern));
        printf("write() resultado: %ld bytes\n", write_ret);
        
        if (write_ret < 0) {
            printf("❌ WRITE FALHOU: %s\n", strerror(errno));
        } else {
            printf("✓ WRITE OK! Verifique os LEDs!\n");
        }
    }
    
    close(fd);
    return 0;
}
EOF

gcc -o test_driver test_driver.c
./test_driver
