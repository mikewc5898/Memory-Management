/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"

sf_prologue *prologue;
sf_epilogue *epilogue;


sf_free_list_node *list_find(size_t sizeIn,int flag){ /*1 if free 0 if malloc*/
    sf_free_list_node *traverse = sf_free_list_head.next;
    while(traverse != &sf_free_list_head){
        if(traverse->size == sizeIn){
            if(flag){
                return traverse;
            }
            else{
                while(traverse->head.links.next == &traverse->head &&
                    traverse->head.links.prev == &traverse->head){
                    traverse = traverse->next;
                    if(traverse == &sf_free_list_head){
                        return &sf_free_list_head;
                    }
                }
                return traverse;
            }
        }
        traverse = traverse->next;
    }
    traverse = &sf_free_list_head;
    if(!(traverse->next == &sf_free_list_head
        && traverse->prev == &sf_free_list_head)){
        while(traverse->size < sizeIn){
            traverse = traverse->next;
            if(traverse == &sf_free_list_head){
                if(flag){
                    return sf_add_free_list(sizeIn,traverse);
                }
                else return &sf_free_list_head;
            }
        }
    }
    if(flag){
        return sf_add_free_list(sizeIn,traverse);
    }
    else{
        while(traverse->head.links.next == &traverse->head &&
            traverse->head.links.prev == &traverse->head){
            traverse = traverse->next;
            if(traverse == &sf_free_list_head){
                return &sf_free_list_head;
            }
        }
        return traverse;
    }
}

void addTo(sf_header *first,sf_free_list_node *pointer){

first->links.next = pointer->head.links.next;
    pointer->head.links.next->links.prev = first;
    pointer->head.links.next = first;
    first->links.prev = &(pointer->head);

}

void removeFrom(sf_header *first,sf_free_list_node *pointer){

    pointer->head.links.next = first->links.next;
    first->links.next->links.prev = &pointer->head;

}

sf_header *coalesce(sf_header* free){
    int prev;
    int next;
    sf_footer *prevFoot;
    sf_header *prevHeader;
    sf_header *nextBlock;
    sf_free_list_node *previous = NULL;
    prev = free->info.prev_allocated;
    prevFoot = (void *)free - sizeof(sf_footer);
    nextBlock = (void *)free + (free->info.block_size << 4);
    next = nextBlock->info.allocated;
    prevHeader = (void *)free - (prevFoot->info.block_size << 4);
    if(free->info.prev_allocated == 0){
        previous = list_find((prevFoot->info.block_size << 4),0);
    }
    sf_free_list_node *current = list_find((free->info.block_size << 4),0);
    sf_free_list_node *next_block = list_find((nextBlock->info.block_size << 4),0);
    if(prev && next){
        if(free->info.allocated == 1){
            sf_free_list_node *new = list_find(free->info.block_size << 4,1);
            addTo(free,new);
        }
        return free;
    }
    else if(!prev && next){
        removeFrom(previous->head.links.next,previous);
        if(free->info.allocated == 0){
            removeFrom(current->head.links.next,current);
        }
        size_t newSize = (prevFoot->info.block_size << 4) + (free->info.block_size << 4);
        prevHeader->info.block_size = newSize >> 4;
        sf_footer *newFoot = (void *)prevHeader + newSize - sizeof(sf_footer);
        newFoot->info = prevHeader->info;
        sf_free_list_node *new = list_find(newSize,1);
        addTo(prevHeader,new);
        return prevHeader;
    }
    else if(prev && !next){
        if(free->info.allocated == 0){
            removeFrom(current->head.links.next,current);
        }
        removeFrom(next_block->head.links.next,next_block);
        size_t newSize = (free->info.block_size << 4) + (nextBlock->info.block_size << 4);
        free->info.block_size = newSize >> 4;
        sf_footer *newFoot = (void *)free + newSize - sizeof(sf_footer);
        newFoot->info = free->info;
        sf_free_list_node *new = list_find(newSize,1);
        addTo(free,new);
        return free;
    }
    else{
        removeFrom(previous->head.links.next,previous);
        if(free->info.allocated == 0){
            removeFrom(current->head.links.next,current);
        }
        removeFrom(next_block->head.links.next,next_block);
        size_t newSize = (prevFoot->info.block_size << 4) + (free->info.block_size << 4) +
        (nextBlock->info.block_size << 4);
        prevHeader->info.block_size = newSize >> 4;
        sf_footer *newFoot = (void *)prevHeader + newSize - sizeof(sf_footer);
        newFoot->info = prevHeader->info;
        sf_free_list_node *new = list_find(newSize,1);
        addTo(prevHeader,new);
        return prevHeader;
    }
}

void init_page(){
    sf_mem_grow();
    prologue = sf_mem_start();
    epilogue =(sf_mem_end() - sizeof(sf_epilogue));
    prologue->header.info.requested_size = 0;
    prologue->header.info.block_size = (0);
    prologue->header.info.prev_allocated = 0;
    prologue->header.info.allocated = 1;
    prologue->footer.info.requested_size = 0;
    prologue->footer.info.block_size = (0);
    prologue->footer.info.prev_allocated = 0;
    prologue->footer.info.allocated = 1;
    epilogue->footer.info.requested_size = 0;
    epilogue->footer.info.block_size = 0;
    epilogue->footer.info.prev_allocated = 0;
    epilogue->footer.info.allocated = 1;
    int blockSize = 4096 - (sizeof(sf_prologue) + sizeof(sf_epilogue));
    //create info for header and footer
    sf_block_info firstNode;
    firstNode.requested_size = 0;
    firstNode.block_size = (blockSize >> 4);
    firstNode.prev_allocated = 0;
    firstNode.allocated = 0;
    //create and link header to free list
    sf_header *first = (sf_mem_start() + sizeof(sf_prologue));
    first->info = firstNode;
    sf_free_list_node *pointer = list_find(blockSize,1);
    addTo(first,pointer);
    //create footer
    //put both into heap
    sf_footer *firstFoot = (void *)first + (first->info.block_size << 4) - sizeof(sf_footer);
    firstFoot->info = firstNode;
}

int add_page(sf_epilogue *Epilogue,size_t size){
    size_t newSize = 0;
    sf_footer *last = (void *)Epilogue - sizeof(sf_footer);
    if(epilogue->footer.info.prev_allocated == 0){
        newSize = last->info.block_size << 4;
    }
    sf_epilogue *changingEpilogue = Epilogue;
    sf_header *header;
    sf_block_info newBlock;

    while(newSize < size){
        newSize = newSize + 4096;
        if(sf_mem_grow() == NULL){
            newSize = newSize - 4096;
            newBlock.requested_size=0;
            newBlock.allocated = 0;
            if (epilogue->footer.info.prev_allocated == 0){
                newBlock.prev_allocated = 0;
            }
            else newBlock.prev_allocated = 1;
            if(epilogue->footer.info.prev_allocated == 0){
                newSize = newSize - (last->info.block_size << 4);
            }
            newBlock.block_size =((newSize) >> 4);
            header = (void* )Epilogue;
            header->info = newBlock;
            sf_footer *foot = (void *)header + (header->info.block_size << 4) - sizeof(sf_footer);
            foot->info = newBlock;
            changingEpilogue = sf_mem_end() - sizeof(sf_epilogue);
            changingEpilogue->footer.info.requested_size = 0;
            changingEpilogue->footer.info.block_size = 0;
            changingEpilogue->footer.info.prev_allocated = 0;
            changingEpilogue->footer.info.allocated = 1;
            epilogue = changingEpilogue;
            sf_free_list_node *newHead = list_find(newSize,1);
            addTo(header,newHead);
            coalesce(header);
            return 0;
        }
        else{
            newBlock.requested_size=0;
            newBlock.allocated = 0;
            if (epilogue->footer.info.prev_allocated == 0){
                newBlock.prev_allocated = 0;
            }
            else newBlock.prev_allocated = 1;
        }
    }
    if(epilogue->footer.info.prev_allocated == 0){
        newSize = newSize - (last->info.block_size << 4);
    }
    newBlock.block_size =((newSize) >> 4);
    header = (void* )Epilogue;
    header->info = newBlock;
    sf_footer *foot = (void *)header + (header->info.block_size << 4) - sizeof(sf_footer);
    foot->info = newBlock;
    changingEpilogue = sf_mem_end() - sizeof(sf_epilogue);
    changingEpilogue->footer.info.requested_size = 0;
    changingEpilogue->footer.info.block_size = 0;
    changingEpilogue->footer.info.prev_allocated = 0;
    changingEpilogue->footer.info.allocated = 1;
    epilogue = changingEpilogue;
    sf_free_list_node *newHead = list_find(newSize,1);
    addTo(header,newHead);
    coalesce(header);
    return 1;
}

int validPointer(void *pp){
    if(pp == NULL){
        return 0;
    }
    else if(pp < (void *)prologue || pp > (void *)epilogue){
        return 0;
    }
    else{
        sf_header *head = (sf_header *)pp;
        if(head->info.allocated == 0){
            return 0;
        }
        else if((head->info.block_size << 4) % 16 !=0){
            return 0;
        }
        else if((head->info.block_size << 4) < 32){
            return 0;
        }
        else if((head->info.requested_size) + 8 > (head->info.block_size << 4)){
            return 0;
        }
        else{
            sf_footer *lastFoot = pp - sizeof(sf_footer);
            if(head->info.prev_allocated == 0 && lastFoot->info.allocated != 0){
                return 0;
            }
            else return 1;
        }
    }
}


void *sf_malloc(size_t size) {
    if(sf_free_list_head.next == &sf_free_list_head
        && sf_free_list_head.prev == &sf_free_list_head){
        init_page();
    }
    if(size == 0){
        return NULL;
    }
    else{
        size_t allocate = size + 8;
        while((allocate % 16 != 0)){
            allocate +=1;
        }
        if(allocate < 32){
            allocate =32;
        }
        sf_free_list_node *found = list_find(allocate,0);
        if(found != &sf_free_list_head){
            sf_header *block = found->head.links.next;
            if(((block->info.block_size << 4) - allocate) < 32){
                removeFrom(block,found);
                block->info.requested_size = size;
                block->info.block_size = (allocate >> 4);
                block->info.allocated = 1;
                //sf_footer *prev_footer = (void*)block - sizeof(sf_footer);
                //if (prev_footer->info.allocated == 0){
                    block->info.prev_allocated = 1;
                //}
                //else block->info.prev_allocated = 1;
                sf_header *adjacent = (void *)block + (block->info.block_size << 4);
                adjacent->info.prev_allocated = 1;
                if(adjacent->info.allocated == 0){
                    sf_footer *adjFoot = (void *)adjacent + (adjacent->info.block_size << 4) - sizeof(sf_footer);
                    adjFoot->info.prev_allocated = 1;
                }
                return &(block->payload);
            }
            else{
                size_t newSize = (block->info.block_size << 4) - allocate;
                removeFrom(block,found);
                block->info.requested_size = size;
                block->info.block_size = (allocate >> 4);
                block->info.allocated = 1;
                //sf_footer *prev_footer = (void*)block - sizeof(sf_footer);
                //if (prev_footer->info.allocated == 0){
                    block->info.prev_allocated = 1;
                //}
                //else block->info.prev_allocated = 1;

                sf_header *newBlock = (void*)block + allocate;
                newBlock->info.allocated = 0;
                newBlock->info.prev_allocated = 1;
                newBlock->info.requested_size = 0;
                newBlock->info.block_size = (newSize >> 4);
                sf_free_list_node *pointer = list_find(newSize,1);
                addTo(newBlock,pointer);
                sf_footer *newFoot = (void *)newBlock + newSize - sizeof(sf_footer);
                newFoot->info.allocated = 0;
                newFoot->info.prev_allocated = 1;
                newFoot->info.requested_size = 0;
                newFoot->info.block_size = (newSize >> 4);
                sf_header *adjacent = (void *)newBlock + (newBlock->info.block_size << 4);
                adjacent->info.prev_allocated = 0;
                if(adjacent->info.allocated == 0){
                    sf_footer *foot = (void *)adjacent + (adjacent->info.block_size << 4) - sizeof(sf_footer);
                    foot->info.prev_allocated = 0;
                }
                return &(block->payload);
            }
        }
        else{
            if(add_page(epilogue,allocate)){
                return sf_malloc(allocate-8);
            }
            else{
                sf_errno = ENOMEM;
                return NULL;
            }
        }
    }
}

void sf_free(void *pp) {
    if(pp != NULL){
        pp = pp-8;
    }
   if(!validPointer(pp)){
        abort();
   }
   else{
    sf_header *head = coalesce((sf_header *)pp);
    head->info.allocated = 0;
    sf_block_info toFooter = head->info;
    sf_footer *newFooter = (void *)head + (head->info.block_size << 4) - sizeof(sf_footer);
    newFooter->info = toFooter;
    sf_header *nextBlock = (void *)head + (head->info.block_size << 4);
    nextBlock->info.prev_allocated = 0;
    if(nextBlock->info.allocated == 0){
        sf_footer *foot = (void *)nextBlock + (nextBlock->info.block_size << 4) - sizeof(sf_footer);
        foot->info.prev_allocated = 0;
    }

   }
}

void *sf_realloc(void *pp, size_t rsize) {
    if(pp != NULL){
        pp = pp-8;
    }
    //void *checked = pp - 8;
    if(!validPointer(pp)){
        sf_errno = EINVAL;
        return NULL;
    }
    else if(rsize == 0){
        pp = pp + 8;
        sf_free(pp);
        return NULL;
    }
    else if((rsize + 8) > (((sf_header *)pp)->info.block_size << 4)){
        sf_header *old = (sf_header *)pp;
        sf_header *new = sf_malloc(rsize) - 8;
        if(new == NULL){
            return NULL;
        }
        else{
            memcpy((void *)new + 8,(void *)old + 8,(new->info.block_size << 4)-8);
            pp = pp + 8;
            sf_free(pp);
            return &(new->payload);
        }
    }
    else{
        size_t alloc_size = rsize + 8;
        while((alloc_size % 16 != 0)){
            alloc_size +=1;
        }
        if(alloc_size < 32){
            alloc_size = 32;
        }
        sf_header *old = (sf_header *)pp;
        if((old->info.block_size << 4) - alloc_size < 32){
            old->info.requested_size = rsize;
            return &(old->payload);
        }
        else{
            size_t newAlloc = ((old->info.block_size << 4) - alloc_size);
            old->info.block_size = alloc_size >> 4;
            old->info.requested_size = rsize;
            sf_header *newFree = (void *)old + alloc_size;
            sf_block_info newInfo;
            newInfo.allocated = 0;
            newInfo.prev_allocated = 1;
            newInfo.requested_size = 0;
            newInfo.block_size = newAlloc >> 4;
            newFree->info = newInfo;
            sf_footer *newFooter = (void *)newFree + (newFree->info.block_size << 4) - sizeof(sf_footer);
            newFooter->info = newInfo;
            sf_free_list_node *pointer = list_find(newAlloc,1);
            addTo(newFree,pointer);
            coalesce(newFree);
            sf_header *adjacent = (void *)newFree + (newFree->info.block_size << 4);
            adjacent->info.prev_allocated = 0;
            if(adjacent->info.allocated == 0){
                sf_footer *foot = (void *)adjacent + (adjacent->info.block_size << 4) - sizeof(sf_footer);
                foot->info.prev_allocated = 0;
            }
            return &(old->payload);
        }
    }
}
