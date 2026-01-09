/******************************************************************************/
/*! @brief      SPI NAND flash format utility and CLI tool
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"

enum {
    LAYER_MIN  = 0,
    LAYER_ANTP_SPEED = LAYER_MIN,
    LAYER_ANTP_POWER,
    LAYER_ANTP_CADENCE,
    LAYER_ANTP_MAX,
    LAYER_GYRO = LAYER_ANTP_MAX,
    LAYER_ACCELEROMETER,
    LAYER_DIM3_MAX,
    LAYER_TEMPERATURE = LAYER_DIM3_MAX,
};

void on_signal(
    int signo
)
{
    printf("on_signal(%d)\n", signo);
    exit(0);
}

// entory point
int main(
    int argc,
    char* argv[]
)
{
    struct timeval tm;
    gettimeofday(&tm, NULL);
    uint64_t prev = tm.tv_sec;

    signal(SIGINT, on_signal);
    printf("start %s\n", argv[0]);

    std::string type,config;
    uint16_t page_s = 0, page_e = 0;
    uint16_t block_s = 0, block_e = 0;
    uint16_t put_string = 0xFFFF;
    uint32_t epoch_s = 0, epoch_e = 0;

    int opt;
    while ((opt = getopt(argc, argv, "t:s:e:S:E:c:p:a:z:")) != -1) {
        switch (opt) {
        case 't': if (optarg != NULL) { type.assign(optarg); } break;
        case 'c': if (optarg != NULL) { config.assign(optarg); } break;
        case 'S': if (optarg != NULL) { page_s = (uint16_t)atoi(optarg); } break;
        case 'E': if (optarg != NULL) { page_e = (uint16_t)atoi(optarg); } break;
        case 's': if (optarg != NULL) { block_s = (uint16_t)atoi(optarg); } break;
        case 'e': if (optarg != NULL) { block_e = (uint16_t)atoi(optarg); } break;
        case 'p': if (optarg != NULL) { put_string = (uint16_t)atoi(optarg); } break;
        case 'a': if (optarg != NULL) { epoch_s = (uint32_t)atoi(optarg); } break;
        case 'z': if (optarg != NULL) { epoch_e = (uint32_t)atoi(optarg); } break;
        default:
            fprintf(stderr, "un-supported flag.(%s), %d\n", argv[0], opt);
            exit(1);
            break;
        }
    }
    if (type.empty()) {
        fprintf(stderr, "required t/flag\n");
        exit(1);
    }
    
    int gpio = open(GPIO, O_RDWR|O_SYNC);
    if (gpio <= 0) {
        printf(
            "could not open device(/dev/gpiomem, %d, %d, %s)\n",
            gpio,
            errno,
            strerror(errno)
        );
        exit(1);
    }
    // GPIO mapping
	uint32_t* gpio_mem = (uint32_t*)mmap(NULL, GPIO_MASK, PROT_READ|PROT_WRITE, MAP_SHARED, gpio, GPIO_BASE);
    if (gpio_mem == MAP_FAILED) {
        printf(
            "could not mmap gpio(%p, %d, %s)\n",
            gpio_mem,
            errno,
            strerror(errno)
        );
        exit(1);
    }
    // WP   -> out(GPIO:5), Low
    gpio_mem[GPFSEL0] &=~(7 << 15); 
    gpio_mem[GPFSEL0] |= (1 << 15);    // output
    gpio_mem[GPCLR0]  =  (1 << 5);     // low

    // HOLD -> out(GPIO:6), High
    gpio_mem[GPFSEL0] &=~(7 << 18); 
    gpio_mem[GPFSEL0] |= (1 << 18);    // output
    gpio_mem[GPSET0]  =  (1 << 6);     // high
    
    // CS   -> out(GPIO:22)
    gpio_mem[GPFSEL2] &=~(7 << 6); 
    gpio_mem[GPFSEL2] |= (1 << 6);     // output
    gpio_mem[GPSET0]  =  (1 << 22);    // high

    // SCLK -> (GPIO:11), ALT0 
    gpio_mem[GPFSEL1] &=~(7 << 3); 
    gpio_mem[GPFSEL1] |= (4 << 3);     // ALT0:SPI0_SCLK  
    gpio_mem[GPSET0]  =  (1 << 11);
    // MOSI -> (GPIO:10), ALT0
    gpio_mem[GPFSEL1] &=~(7 << 0); 
    gpio_mem[GPFSEL1] |= (4 << 0);     // ALT0:SPI0_MOSI
    gpio_mem[GPSET0]  =  (1 << 10);
    // MISO -> (GPIO:9), ALT0
    gpio_mem[GPFSEL0] &=~(7 << 27); 
    gpio_mem[GPFSEL0] |= (4 << 27);    // ALT0:SPI0_MISO
    gpio_mem[GPSET0]  =  (1 << 9);
#if 0 
    // CE0(CS)-> (GPIO:8), ALT0
    gpio_mem[GPFSEL0] &=~(7 << 24); 
    gpio_mem[GPFSEL0] |= (4 << 24);    // ALT0:SPI0_CE0_N
#endif
    int spi = open(SPI0, O_RDWR);
    if (spi <= 0) {
        printf("could not open device,(%s)\n", SPI0);
        exit(1);
    }
    uint32_t wr_mode = 0;
    uint8_t bits_per_word = 8;
    uint32_t max_speed = 80000000;
    if ((ioctl(spi, SPI_IOC_WR_MODE32, &wr_mode) < 0)
        || (ioctl(spi, SPI_IOC_RD_MODE32, &wr_mode) < 0)
        || (ioctl(spi, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0)
        || (ioctl(spi, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word) < 0)
        || (ioctl(spi, SPI_IOC_WR_MAX_SPEED_HZ, &max_speed) < 0)
        || (ioctl(spi, SPI_IOC_RD_MAX_SPEED_HZ, &max_speed)  < 0))
    {
        printf("Could not set SPI params");
        exit(1);
    }
    uint8_t status = DEVICE_RESET;
    uint8_t deviceid[32] = { 0x00, };

    // software reset.
    uint8_t cmd[2112] = { DEVICE_RESET, };
    gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(spi, "reset", cmd, 1);
    gpio_mem[GPSET0] =  (1 << 22);

    usleep(1000);
    
    // device id
    cmd[0] = JEDEC_ID;
    cmd[1] = 0;

    gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(spi, "device id", cmd, 2);
    memset(cmd, 0, sizeof(cmd));
    spi_read(spi, cmd, 3);
    gpio_mem[GPSET0] =  (1 << 22);

    printf(
        "device(%02X %02X %02X)\n",
        cmd[0],
        cmd[1],
        cmd[2]
    );

    // W25N01G
    if (cmd[0] != 0xEF || cmd[1] != 0xAA || cmd[2] != 0x21) {
        exit(1);
    }

    // origin block 0, page 0
    unsigned block = 0, page = 0;
    int ret;

    // un-protect
    cmd[0] = WRITE_STATUS_REGISTER_L;
    cmd[1] = STATUS_REGISTER_1_ADDR;
    cmd[2] = 0;
    gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(spi, "un-protect", cmd, 3);
    gpio_mem[GPSET0] =  (1 << 22);
    usleep(100);
    
    // write enable 
    cmd[0] = WRITE_ENABLE;
    gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(spi, "write enable", cmd, 1);
    gpio_mem[GPSET0] =  (1 << 22);
    usleep(100);

    // read initialize
    status = DEVICE_RESET;
    if (type.compare("erase") != 0) {
        for (block = 0; block < 1024;) {
            ret = read_initialize(
                spi,
                gpio_mem,
                &status,
                block
            );
            if (ret == READ_WITH_DATA) {
                block++;
            } else if (ret == READ_INVALID_BLOCK) {
                exit(1);
            }
        }
    }
    printf(
        "command : %s/BLOCK : %u -> %u/PAGE : %u -> %u\n",
        type.c_str(),
        block_s, block_e,
        page_s, page_e
    );

    // read page, and block between start and end. 
    if (type.compare("read") == 0) {
        status = DEVICE_RESET;
        for (block = block_s; block < block_e; block++) {
            for (page = page_s; page < page_e; page++) {
                while(1) {
                    ret = read_by_block_page(
                        spi,
                        gpio_mem,
                        &status,
                        block,
                        page,
                        cmd,
                        sizeof(cmd) 
                    );
                    if (ret == READ_WITH_DATA) { break; }
                }
                printf(">>>> read >>>>\n");
                for (auto n = 0; n < sizeof(cmd); n++) {
                    printf("%02X ", cmd[n]);
                    if ((n+1) % 32  == 0) {
                        printf("\n");
                    } else if ((n+1) % 8 == 0) {
                        printf(":");
                    }
                }
                printf("<<<<\n");
                if (put_string != 0xFFFF) {
                    printf("%s\n", &cmd[put_string]);
                }
            }
        }
    } else if (type.compare("erase") == 0) {
        status = DEVICE_RESET;
        for (block = block_s; block < block_e; block++) {
            while(1) {
                ret = erase_block(
                    spi,
                    gpio_mem,
                    &status,
                    block
                );
                if (ret == ERASE_DONE) { break; }
            }
            printf(">>>> erase(%4d) >>>>\n", block);
        }
    } else if (type.compare("format") == 0) {
        status = DEVICE_RESET;
        for (block = 0; block < 1024; block++) {
            while(1) {
                ret = erase_block(
                    spi,
                    gpio_mem,
                    &status,
                    block
                );
                if (ret == ERASE_DONE) { break; }
            }
            for (page = 0; page < 64; page++) {
                while(1) {
                    ret = read_by_block_page(
                        spi,
                        gpio_mem,
                        &status,
                        block,
                        page,
                        cmd,
                        sizeof(cmd) 
                    );
                    if (ret == READ_WITH_DATA) { break; }
                }
                for (auto n = 0; n < 16; n++) {
                    if (cmd[n] != 0xff) {
                        printf("failed, format-full\n");
                        exit(1);
                    }
                }
            }
            printf(">>>> format(%4d) >>>>\n", block);
        }
    } else if (type.compare("check") == 0) {
        status = DEVICE_RESET;
        for (block = 0; block < 1024; block++) {
            for (page = 0; page < 64; page++) {
                while(1) {
                    ret = read_by_block_page(
                        spi,
                        gpio_mem,
                        &status,
                        block,
                        page,
                        cmd,
                        sizeof(cmd) 
                    );
                    if (ret == READ_WITH_DATA) { break; }
                }
                unsigned magic = (0xdeadc0de);
                if (memcmp(cmd, &magic, sizeof(magic)) == 0) {
                    ;; // ok
                } else {
                    for (auto n = 0; n < 16; n++) {
                        if (cmd[n] != 0xff) {
                            printf("failed, check\n");
                            exit(1);
                        }
                    }
                }

            }
            printf(">>>> check(%4d) >>>>\n", block);
        }
    } else if (type.compare("config") == 0) {
        status = DEVICE_RESET;
        if (!config.empty() && (config.size() < (sizeof(cmd) - 4))) {
            memset(cmd, 0, sizeof(cmd));
            compress_ptr cp = (compress_ptr)cmd;
            cp->magic = MAGIC;
            cp->type = LAYER_CONFIG; 
            cp->ant.length = config.size();

            memcpy(
                (cp+1),
                config.c_str(),
                MIN(config.size(), sizeof(cmd) - 4)
            );
            printf(
                "config : %s(%d)%d ,, %d\n",
                config.c_str(),
                config.size(),
                cp->ant.length,
                sizeof(*cp)
            );
            while(1) {
                ret = write_block_page(
                        spi,
                        gpio_mem,
                        &status,
                        0,
                        0,
                        cmd,
                        sizeof(cmd) - 64 
                    );
                if (ret == WRITE_DONE) { break; }
                usleep(1000);
            }
        }
    } else if (type.compare("write") == 0) {
        for (block = block_s; block < block_e; block++) {
            for (page = page_s; page < page_e; page++) {
                status = DEVICE_RESET;
                memcpy(&cmd[0], &block, sizeof(block));
                memcpy(&cmd[2], &page, sizeof(page));
                for (auto n = (sizeof(block) + sizeof(page));n < sizeof(cmd); n++) {
                    cmd[n] = n;
                }
                compress_ptr cp = (compress_ptr)cmd;
                cp->magic = MAGIC;
                while(1) {
                    ret = write_block_page(
                        spi,
                        gpio_mem,
                        &status,
                        block,
                        page,
                        cmd,
                        sizeof(cmd) - 64 
                    );
                    if (ret == WRITE_DONE) { break; }
                    usleep(1000);
                }
            }
        }
    } else if (type.compare("csv") == 0) {
        for (block = block_s; block < block_e; block++) {
            for (page = page_s; page < page_e; page++) {
                uint8_t decbf[4096] = { 0x00, };
                std::vector<int16_t> gyrv(4096);
                std::vector<int16_t> accv(4096);
                std::vector<int16_t> tmpv(4096);
                int gyrvcnt = 0, accvcnt = 0, tmpvcnt = 0;
                uint64_t gyrvtime = 0, accvtime = 0, tmpvtime = 0,
                    gyrvstep = 0, accvstep = 0, tmpvstep = 0;
                status = DEVICE_RESET;
                while(1) {
                    ret = read_by_block_page(
                        spi,
                        gpio_mem,
                        &status,
                        block,
                        page,
                        decbf,
                        sizeof(decbf) 
                    );
                    if (ret == READ_WITH_DATA) { break; }
                }
                printf(">>>> read >>>>\n");
                for (auto n = 0; n < sizeof(decbf); n++) {
                    printf("%02X ", decbf[n]);
                    if ((n+1) % 32  == 0) {
                        printf("\n");
                    } else if ((n+1) % 8 == 0) {
                        printf(":");
                    }
                }
                printf("<<<<\n");
                int decoded_bytes = 0;

                printf(">>>>csv\n");

                for(auto offset = 0; offset < sizeof(decbf);) {
                    compress_ptr cp = (compress_ptr)&decbf[offset];
                    if (cp->magic != MAGIC) {
                        // printf("!=magic(%d)\n", decoded_bytes);
                        break;
                    }

                    auto bp = (offset + sizeof(*cp));
                    switch(cp->type) {
                    case LAYER_ANTP_SPEED:
                        printf(
                            "rec\tspeed(%d)\t%llu\t%02X %02X %02X %02X: %02X %02X %02X %02X:"
                            "%02X %02X %02X %02X: %02X %02X %02X %02X: %02X %02X\n",
                            cp->ant.length,
                            (((uint64_t)cp->time*1000) + (uint64_t)cp->msec),
                            decbf[bp + 0], decbf[bp + 1], decbf[bp + 2], decbf[bp + 3],
                            decbf[bp + 4], decbf[bp + 5], decbf[bp + 6], decbf[bp + 7],
                            decbf[bp + 8], decbf[bp + 9], decbf[bp +10], decbf[bp +11],
                            decbf[bp +12], decbf[bp +13], decbf[bp +14], decbf[bp +15],
                            decbf[bp +16], decbf[bp +17] 
                        );
                        offset += cp->ant.length;
                        decoded_bytes += (18 + 4);
                        break;
                    case LAYER_ANTP_POWER:
                        printf(
                            "rec\tpower(%d)\t%llu\t%02X %02X %02X %02X: %02X %02X %02X %02X:"
                            "%02X %02X %02X %02X: %02X %02X %02X %02X: %02X %02X\n",
                            cp->ant.length,
                            (((uint64_t)cp->time*1000) + (uint64_t)cp->msec),
                            decbf[bp + 0], decbf[bp + 1], decbf[bp + 2], decbf[bp + 3],
                            decbf[bp + 4], decbf[bp + 5], decbf[bp + 6], decbf[bp + 7],
                            decbf[bp + 8], decbf[bp + 9], decbf[bp +10], decbf[bp +11],
                            decbf[bp +12], decbf[bp +13], decbf[bp +14], decbf[bp +15],
                            decbf[bp +16], decbf[bp +17]
                        );
                        offset += cp->ant.length;
                        decoded_bytes += (18 + 4);
                        break;
                    case LAYER_ANTP_CADENCE:
                        printf(
                            "rec\tcadence(%d)\t%llu\t%02X %02X %02X %02X: %02X %02X %02X %02X:"
                            "%02X %02X %02X %02X: %02X %02X %02X %02X: %02X %02X\n",
                            cp->ant.length,
                            (((uint64_t)cp->time*1000) + (uint64_t)cp->msec),
                            decbf[bp + 0], decbf[bp + 1], decbf[bp + 2], decbf[bp + 3],
                            decbf[bp + 4], decbf[bp + 5], decbf[bp + 6], decbf[bp + 7],
                            decbf[bp + 8], decbf[bp + 9], decbf[bp +10], decbf[bp +11],
                            decbf[bp +12], decbf[bp +13], decbf[bp +14], decbf[bp +15],
                            decbf[bp +16], decbf[bp +17]
                        );
                        offset += cp->ant.length;
                        decoded_bytes += (18 + 4);
                        break;
                    case LAYER_GYRO: {
                        printf("head\tgyro(%llu, %d, %d, %d)\n",
                            (((uint64_t)cp->time*1000) + (uint64_t)cp->msec),
                            cp->gyr.count,
                            cp->gyr.dim,
                            gyrvcnt);
                        gyrvtime = (((uint64_t)cp->time*1000) + (uint64_t)cp->msec);
                        gyrvstep = cp->step;
                        for(int n = 0; n < cp->gyr.count; n++) {
                            gyrv.at((n*3) + 0) = ((int16_t*)&decbf[bp + (n * sizeof(int16_t) * 3)])[0];
                            gyrv.at((n*3) + 1) = ((int16_t*)&decbf[bp + (n * sizeof(int16_t) * 3)])[1];
                            gyrv.at((n*3) + 2) = ((int16_t*)&decbf[bp + (n * sizeof(int16_t) * 3)])[2];
                        }
                        gyrvcnt += cp->gyr.count;
                        offset += (cp->gyr.count * sizeof(int16_t) * 3);
                        decoded_bytes += ((cp->gyr.count * sizeof(int16_t) * 3) + 4);
                        } break;
                    case LAYER_ACCELEROMETER: {
                        printf("head\taccelerometer(%llu, %d, %d, %d)\n",
                            (((uint64_t)cp->time*1000) + (uint64_t)cp->msec),
                            cp->gyr.count,
                            cp->gyr.dim,
                            accvcnt);
                        accvtime = (((uint64_t)cp->time*1000) + (uint64_t)cp->msec);
                        accvstep = cp->step;
                        for(int n = 0; n < cp->gyr.count; n++) {
                            accv.at((n*3) + 0) = ((int16_t*)&decbf[bp + (n * sizeof(int16_t) * 3)])[0];
                            accv.at((n*3) + 1) = ((int16_t*)&decbf[bp + (n * sizeof(int16_t) * 3)])[1];
                            accv.at((n*3) + 2) = ((int16_t*)&decbf[bp + (n * sizeof(int16_t) * 3)])[2];
                        }
                        accvcnt += cp->gyr.count;
                        offset += (cp->gyr.count * sizeof(int16_t) * 3);
                        decoded_bytes += ((cp->gyr.count * sizeof(int16_t) * 3) + 4);
                        } break;
                    case LAYER_TEMPERATURE: {
                        printf("head\ttemprature(%llu, %d)\n",
                            (((uint64_t)cp->time*1000) + (uint64_t)cp->msec),
                            cp->tmp.count);
                        tmpvtime = (((uint64_t)cp->time*1000) + (uint64_t)cp->msec);
                        tmpvstep = cp->step;
                        for(int n = 0; n < cp->tmp.count; n++) {
                            tmpv.at(n) = ((int16_t*)&decbf[bp + (n * sizeof(int16_t) * 1)])[0];
                        }
                        tmpvcnt += cp->tmp.count;
                        offset += (cp->tmp.count * sizeof(int16_t));
                        decoded_bytes += ((cp->tmp.count * sizeof(int16_t)) + 4);
                        } break;
                    default:
                        printf("not support\n");
                        break;
                    }
                    offset += sizeof(*cp);
                }
                if (gyrvcnt > 0) {
                    for(auto n = 0; n < gyrvcnt; n+=3) {
                        printf(
                            "rec\tgyro(%03d)\t%llu\t%06d\t%06d\t%06d\n",
                            n,
                            gyrvtime + (gyrvstep * n),
                            gyrv.at(n + 0), gyrv.at(n + 1), gyrv.at(n + 2)
                        );
                    }
                }
                if (accvcnt > 0) {
                    for(auto n = 0; n < accvcnt; n+=3) {
                        printf(
                            "rec\taccel(%03d)\t%llu\t%06d\t%06d\t%06d\n",
                            n,
                            accvtime + (accvstep * n),
                            accv.at(n + 0), accv.at(n + 1), accv.at(n + 2)
                        );
                    }
                }
                if (tmpvcnt > 0) {
                    for(auto n = 0; n < tmpvcnt; n++) {
                        printf(
                            "rec\ttemp(%03d)\t%llu\t%06d\n",
                            n,
                            tmpvtime + (tmpvstep * n),
                            tmpv.at(n + 0)
                        );
                    }
                }
            }
        }
    } else if (type.compare("search") == 0) {
        std::map<uint32_t, std::vector<uint32_t> > timestamps;
        for (block = 0; block < 1024; block++) {
            for (page = 0; page < 64; page++) {
                status = DEVICE_RESET;
                while(1) {
                    ret = read_by_block_page(
                        spi,
                        gpio_mem,
                        &status,
                        block,
                        page,
                        cmd,
                        sizeof(cmd) 
                    );
                    if (ret == READ_WITH_DATA) { break; }
                }
                compress_ptr cp = (compress_ptr)&cmd[0];
                if (cp->magic != MAGIC) {
                    break;
                }
                for(auto offset = 0; offset < sizeof(cmd);) {
                    compress_ptr cp = (compress_ptr)&cmd[offset];
                    if (cp->magic != MAGIC) { break; }
                    auto bp = (offset + sizeof(*cp));
                    switch(cp->type) {
                    case LAYER_ANTP_SPEED:      offset += cp->ant.length;   break;
                    case LAYER_ANTP_POWER:      offset += cp->ant.length;   break;
                    case LAYER_ANTP_CADENCE:    offset += cp->ant.length;   break;
                    case LAYER_GYRO:            offset += (cp->gyr.count * sizeof(int8_t)); break;
                    case LAYER_ACCELEROMETER:   offset += (cp->gyr.count * sizeof(int8_t)); break;
                    case LAYER_TEMPERATURE:     offset += (cp->tmp.count * sizeof(int8_t)); break;
                    default:                    printf("not support\n");    break;
                    }
                    offset += sizeof(*cp);

                    // timestamps
                    if (cp->time && cp->time >= epoch_s && cp->time <= epoch_e) {
                        uint32_t blockpage = (((((uint32_t)block)<<16)&0xFFFF0000)|(((uint32_t)page)&0x0000FFFF));
                        auto it = timestamps.find(blockpage);
                        if (it != timestamps.end()) {
                            auto found = std::find_if(
                                (it->second).begin(),
                                (it->second).end(),
                                [cp](uint32_t x) { return(x == cp->time); });
                            if (found == (it->second).end()) {
                                (it->second).push_back(cp->time);
                            }
                        } else {
                            timestamps[blockpage] = { cp->time };
                        }
                    }
                }
            }
        }
        printf("search>>\n");
        for(auto& it: timestamps) {
            block = (uint16_t)(((it.first)>>16)&0xFFFF);
            page = (uint16_t)((it.first)&0xFFFF);
            printf("block:%3u, page:%4u [%4u]\n\t", block, page, (unsigned)(it.second).size());
            uint32_t counter = 0;
            for(auto& v: (it.second)) {
                printf(" %08x", v);
                if ((++counter) % 8 == 0) {
                    printf("\n\t");
                }
            }
            printf("\n");
        }
    } else {
        printf("not support.\n");
    }

    munmap(gpio_mem, GPIO_MASK);
    close(gpio);
    close(spi);

    printf("fin %s\n", argv[0]);
    return(0);
}


