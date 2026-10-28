#include "DepthFirstIterator.h"
#include <vector>
#include "WorkItem.h"
using namespace std;

DepthFirstIterator::DepthFirstIterator(WorkItem* root)
{
    this->root = root;
    this->currentItem = nullptr;
    first();
}

void DepthFirstIterator::first()
{
    //clear stack first
    while(!this->pending.empty())
    {
        this->pending.pop();
    }
    
    if(this->root != nullptr)
    {
        this->pending.push(this->root);
        this->currentItem = this->root;
    }
    else
    {
        this->currentItem = nullptr;
    }

}

void DepthFirstIterator::next()
{
    if(this->pending.empty())
    {
        return;
    }

    WorkItem* current = this->pending.top();
    this->pending.pop();

    vector<WorkItem*> children = current->getChildren();
    int size = children.size();

    for(int i = size - 1; i >= 0; i--)
    {
        this->pending.push(children[i]);
    }

    if(!this->pending.empty())
        this->currentItem = this->pending.top();
    else
        this->currentItem = nullptr;

}

bool DepthFirstIterator::isDone()
{
    return this->pending.empty();

}

WorkItem* DepthFirstIterator::current()
{
    return this->currentItem;
}