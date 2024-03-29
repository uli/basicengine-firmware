#ifndef LIB_SCP_QLIST
#define LIB_SCP_QLIST

#ifndef NULL
  #define NULL 0
#endif

template<class T>
class QList
{
private:
  typedef struct node
  {
    T item;
    node *next,*prev;
  }node;

  int len; // Size of list 
  node *start,*end; // Pointers to start and end

public:
  QList()
  {
    len = 0;
    start = NULL;
    end = NULL;
  }

  ~QList()
  {
    clear();
  }

  // Push at front
  bool push_front(const T i)
  {
    node *tmp = new node;
    if (!tmp)
      return false;
    tmp->item = i;
    tmp->next = NULL;
    tmp->prev = NULL;

    if(start==NULL) // If list is empty
    {
      start = tmp;
      end = tmp;
    }
    else // Insert at start
    {
      tmp->next = start;
      start->prev = tmp;
      start = tmp;
    }
    len++; // Increase size counter
    return true;
  }

  // Push at back
  bool push_back(const T i)
  {
    node *tmp = new node;
    if (!tmp)
      return false;
    tmp->item = i;
    tmp->next = NULL;
    tmp->prev = NULL;

    if(end==NULL) // If list is empty
    {
      start = tmp;
      end = tmp;
    }
    else // Insert at the end
    {
      tmp->prev = end;
      end->next = tmp;
      end = tmp;
    }
    len++; // Increase size counter
    return true;
  }

  // Pop from front
  void pop_front()
  {
    if(start!=NULL)
    {
      node *tmp = start;
      start = start->next;
      if(start!=NULL) // Re-link next item to NULL
        start->prev = NULL;
      else // List became empty so we need to clear end
        end = NULL;
      delete tmp;
      len--; // Decrease counter
    }
  }

  // Pop from back
  void pop_back()
  {
    if(end!=NULL)
    {
      node *tmp = end;
      end = end->prev;
      if(end!=NULL) //Re-link previous item to NULL
        end->next = NULL;
      else // List became empty so we need to clear start
        start = NULL;
      delete tmp;
      len--; // Decrease counter
    }
  }

  void remove(const T item)
  {
    node *tmp = start;
    while (tmp) {
      node *next = tmp->next;
      node *prev = tmp->prev;

      if (tmp->item == item) {
        if (prev)
          prev->next = next;
        else
          start = next;

        if (next)
          next->prev = prev;
        else
          end = prev;

        delete tmp;
        len--;

        return;
      }
      tmp = next;
    }
  }

  // Get item from front
  T front()
  {
    if(start!=NULL)
      return start->item;
    //TODO: Catch error when list is empty
    for (;;);
  }

  //Get item from back
  T back()
  {
    if(end!=NULL)
      return end->item;
    //TODO: Catch error when list is empty
    for (;;);
  }

  // Get size
  int size()
  {
    return this->len;
  }

  // Clear list
  void clear()
  {
    node *tmp = start;
    while(start!=NULL)
    {
      tmp = start;
      start = start->next;
      delete tmp; // Delete item
      len--; // Decrease counter
    }
    end = NULL;
  }
  void clear(unsigned int index)
  {
    node *tmp = start;
    for(unsigned int i=0;i<=index&&tmp!=NULL;i++)
    {
      if(i==index)
      {
        if(tmp->prev!=NULL)
          tmp->prev->next = tmp->next;
        else
          start = tmp->next;

        if(tmp->next!=NULL)
          tmp->next->prev = tmp->prev;
        else
          end = tmp->prev;

        len--; // Decrease counter
        delete tmp; // Delete item
        break;
      } else
        tmp = tmp->next;
    }
  }

  // Get at index
  T get(unsigned int index)
  {
    node *tmp = start;
    for(unsigned int i=0;i<=index&&tmp!=NULL;i++)
    {
      if(i==index)
        return tmp->item;
      else
        tmp=tmp->next;
    }
    //TODO: Catch error when index is out of range
  }

  T& at(unsigned int index)
  {
    node *tmp = start;
    for(unsigned int i=0;i<=index&&tmp!=NULL;i++)
    {
      if(i==index)
        return tmp->item;
      else
        tmp=tmp->next;
    }
    //TODO: Catch error when index is out of range
    for (;;);
  }

  // Get length
  int length()
  {
    return this->len;
  }

  // Get index of value
  int indexOf(T val)
  {
    for(int i=0;i<this->size();i++)
      if(this->at(i) == val)
        return i;
    return -1;
  }

  // Array operators
  T& operator[](unsigned int index)
  {
    node *tmp = start;
    for(unsigned int i=0;i<=index&&tmp!=NULL;i++)
    {
      if(i==index)
        return tmp->item;
      else
        tmp=tmp->next;
    }
    //TODO: Catch error when index is out of range
    for (;;);
  }


  const T& operator[](unsigned int index) const
  {
    node *tmp = start;
    for(unsigned int i=0;i<=index&&tmp!=NULL;i++)
    {
      if(i==index)
        return tmp->item;
      else
        tmp=tmp->next;
    }
    //TODO: Catch error when index is out of range
    for (;;);
  }
};
#endif
