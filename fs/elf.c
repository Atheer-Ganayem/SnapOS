#include <elf.h>
#include <vfs.h>
#include <asm/elf.h>
#include <stdbool.h>
#include <string.h>
#include <mm.h>

static bool validate_ident(struct elf_ident* ident) {
  if (ident->magic[0] != ELF_MAGIC_0 || ident->magic[1] != ELF_MAGIC_1 ||
      ident->magic[2] != ELF_MAGIC_2 || ident->magic[3] != ELF_MAGIC_3)
    return false;

  // for now our elf parser only supports 64 bit.
  if (ident->class != ELF_CLASS64) {
    return false;
  }

  return ident->class == ELF_TARGET_CLASS && ident->data == ELF_TARGET_DATA &&
          ident->version == ELF_VERSION_CURRENT;
}

static int load_program_header(struct task_struct* t, struct file* f, struct elf_program_header_64* phdr) {
  if (phdr->type != ELF_PHDR_LOAD) return 0;
  if ((phdr->vaddr & (phdr->align - 1)) || (phdr->paddr & (phdr->align - 1))) return 0;

  pgprot_t prot = {.prot_flags = PAGE_USER | PAGE_RW, .allow_overwrite = false};

  if (!(phdr->flags & ELF_PHDR_FLAG_EXEC)) prot.prot_flags |= PAGE_NO_EXECUTE;
  if (phdr->flags & ELF_PHDR_FLAG_WRITE) prot.prot_flags |= PAGE_RW;

  uint64_t mem_size = phdr->mem_size + (phdr->vaddr & (PAGE_SIZE - 1));

  ssize_t page_count = vmm_alloc_range(t->mm->pgd, (void*)PAGE_ALIGN_DOWN(phdr->vaddr), mem_size, prot);
  if (page_count < 0) {
    return (int)page_count;
  }

  if ((size_t)vfs_seek(f, phdr->offset, SEEK_SET) != phdr->offset) {
    return -EINVAL;
  }

  ssize_t ret = vfs_read(f, (void*)phdr->vaddr, phdr->file_size);
  if (ret < 0) {
    return (int)ret;
  }

  if (phdr->mem_size > phdr->file_size) {
    memset((void*)(phdr->vaddr + phdr->file_size), 0x00, phdr->mem_size - phdr->file_size);
  }

  mm_add_alloc(t->mm, (void*)PAGE_ALIGN_DOWN(phdr->vaddr), page_count);

  return 0;
}

void* elf_load(const char* path, struct task_struct* t) {
  struct file* f = vfs_open(path);
  if (IS_ERR(f)) {
    return f;
  }

  struct elf_header_64 eh;

  ssize_t ret = vfs_read(f, &eh, sizeof(struct elf_header_64));
  if (ret != sizeof(struct elf_header_64)) {
    vfs_close(f);
    return ERR_PTR(-EINVAL);
  }

  if (!validate_ident(&eh.ident) || eh.machine != ELF_TARGET_MACHINE || eh.type != ELF_TYPE_EXEC) {
    vfs_close(f);
    return ERR_PTR(-EINVAL);
  }

  ssize_t res = vfs_seek(f, eh.phoff, SEEK_SET);
  if (res < 0) {
    vfs_close(f);
    return ERR_PTR(res);
  }

  struct elf_program_header_64* phdr_base = kmalloc(sizeof(struct elf_program_header_64) * eh.phnum);
  if (!phdr_base) {
    vfs_close(f);
    return ERR_PTR(-ENOMEM);
  }

  ret = vfs_read(f, phdr_base, sizeof(struct elf_program_header_64) * eh.phnum);
  if ((size_t)ret != (sizeof(struct elf_program_header_64) * eh.phnum)) { // TODO: same as the todo above
    vfs_close(f);
    kfree(phdr_base);
    return ERR_PTR(ret);
  }

  struct elf_program_header_64* phdr = phdr_base;

  for (uint32_t i = 0; i < eh.phnum; i++, phdr++) {
    int res = load_program_header(t, f, phdr);
    if (res < 0) {
      vfs_close(f);
      kfree(phdr_base);
      return ERR_PTR(res);
    }
  }

  kfree(phdr_base);
  vfs_close(f);
  return (void*)eh.entry;
}