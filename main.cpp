#include <iostream>
#include <cstring>

#include <ext2fs/ext2_fs.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

// Anpassen!
#define IMAGE_PATH "/home/johann/CLionProjects/ext2reader_git/bsyfile"

#define BASE_OFFSET 1024
#define BLOCK_OFFSET(block) (BASE_OFFSET + (block - 1) * block_size)

unsigned int block_size = 0;

static void read_inode(int fd, unsigned long inode_no, const struct ext2_group_desc *group, struct ext2_inode *inode) {
    auto ret = lseek(fd, BLOCK_OFFSET(group->bg_inode_table) + (inode_no - 1) * sizeof(struct ext2_inode), SEEK_SET);
    read(fd, inode, sizeof(struct ext2_inode));

    std::cout << "lseek returned " << ret << std::endl;
}


static void read_dir(int fd, const struct ext2_inode *inode, const struct ext2_group_desc *group) {
    void *block;

    if (S_ISDIR(inode->i_mode)) {
        std::cout << "Mode: " << inode->i_mode << std::endl;
        struct ext2_dir_entry_2 *entry;
        unsigned int size = 0;

        if ((block = malloc(block_size)) == nullptr) {
            std::cout << "Speicherfehler!";
            close(fd);
            exit(1);
        }

        lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_DATA);
        read(fd, block, block_size);

        entry = (struct ext2_dir_entry_2 *) block;
        while ((size < inode->i_size) && entry->inode) {
            char file_name[EXT2_NAME_LEN + 1];
            memcpy(file_name, entry->name, entry->name_len);
            file_name[entry->name_len] = 0;
            std::cout << "\t" << entry->inode << "\t\t\t" << file_name << std::endl;
            entry = static_cast<ext2_dir_entry_2 *>((void *) entry + entry->rec_len);
            size += entry->rec_len;
        }

        free(block);
        std::cout << std::endl;
    } else {
        std::cout << "Something went wrong! \ni_mode: " << inode->i_mode << std::endl;
        std::cout << "i_blocks " << inode->i_blocks << std::endl;
        std::cout << "i_size " << inode->i_size << std::endl;
        std::cout << "bg_used_dirs_count " << group->bg_used_dirs_count << std::endl;


    }
}

int main() {
    struct ext2_super_block super_block{};
    struct ext2_group_desc group_desc{};
    struct ext2_inode inode{};

    int fd;

    if ((fd = open(IMAGE_PATH, O_RDONLY)) < 0) {
        perror(IMAGE_PATH);
        exit(1);
    }


    lseek(fd, BASE_OFFSET, SEEK_SET);
    read(fd, &super_block, sizeof(super_block));

    if (super_block.s_magic != EXT2_SUPER_MAGIC) {
        std::cout << "Kein Ext2-Dateisystem!" << std::endl;
        exit(1);
    }

    block_size = static_cast<unsigned int>(BASE_OFFSET << super_block.s_log_block_size);

    lseek(fd, BASE_OFFSET + block_size, SEEK_SET);
    read(fd, &group_desc, sizeof(group_desc));

    read_inode(fd, 2, &group_desc, &inode); // 2 ist root
    read_dir(fd, &inode, &group_desc);

    //todo: bug -- Verknüpfung oder so? Ich bin sprachlos!
    group_desc = ext2_group_desc{};
    inode = ext2_inode{};

    read_inode(fd, 16385, &group_desc, &inode); // 2 ist root

    std::cout << "\t i_size " << inode.i_size << std::endl
              << "\t i_mtime " << inode.i_mtime << std::endl;


    read_dir(fd, &inode, &group_desc);


    close(fd);
    exit(0);
}

