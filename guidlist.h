// double-linked list template class, cloned from Borland C++'s
// template implementation with the same name.

#ifndef __GUIDLIST_H__
#define __GUIDLIST_H__

template <class T> class TDoubleListImp 
{
  struct element
  {
    T that;
    element *prev, *next;

    // constructor
    inline element(const T &t, element *Prev = NULL, element *Next = NULL) :
      that(t), prev(Prev), next(Next) {};
  };

  element *head, *tail;
  int elementcount;
public:
  inline TDoubleListImp();
  ~TDoubleListImp();

  typedef void (*IterFunc) (T &, void *);
  typedef int (*CondFunc) (const T &, void *);

  inline int GetItemsInContainer() const;
  inline void AddAtHead(const T &t);
  inline void AddAtTail(const T &t);
  inline void Flush();
  inline bool IsEmpty() const;
  inline const T& PeekHead() const;
  inline const T& PeekTail() const;
  inline void ForEach(IterFunc iter, void *args);
  inline T *FirstThat(CondFunc cond, void *args);
  inline T *LastThat(CondFunc cond, void *args);
  inline void DetachAtHead();
  inline void DetachAtTail();
};

template <class T>
TDoubleListImp<T>::TDoubleListImp() :
  head(NULL), tail(NULL), elementcount(0)
{
}

template <class T>
TDoubleListImp<T>::~TDoubleListImp()
{
  while (head)
  {
    element *next = head->next;
    delete head;
    head = next;
  }
}

template <class T>
int TDoubleListImp<T>::GetItemsInContainer() const
{
  return elementcount;
}

template <class T>
void TDoubleListImp<T>::AddAtHead(const T &t)
{
  element *oldhead = head;
  head = new element(t, NULL, oldhead);
  if (oldhead == NULL) tail = head;
  else oldhead->prev = head;
  elementcount++;
}

template <class T>
void TDoubleListImp<T>::AddAtTail(const T &t)
{
  element *oldtail = tail;
  tail = new element(t, oldtail, NULL);
  if (oldtail == NULL) head = tail;
  else oldtail->next = tail;
  elementcount++;
}

template <class T>
void TDoubleListImp<T>::Flush()
{
  while (head)
  {
    element *next = head->next;
    delete head;
    head = next;
  }
  head = tail = NULL;
  elementcount = 0;
}

template <class T>
bool TDoubleListImp<T>::IsEmpty() const
{
  return (elementcount == 0);
}

template <class T>
const T& TDoubleListImp<T>::PeekHead() const
{
  return head->that;       // error if list is empty
}

template <class T>
const T& TDoubleListImp<T>::PeekTail() const
{
  return tail->that;       // error if list is empty
}

template <class T>
void TDoubleListImp<T>::ForEach(IterFunc iter, void *args)
{
  for (element *ptr = head; ptr; ptr = ptr->next)
    iter(ptr->that, args);
}

template <class T>
T *TDoubleListImp<T>::FirstThat(CondFunc cond, void *args)
{
  for (element *ptr = head; ptr; ptr = ptr->next)
    if (cond(ptr->that, args)) return &ptr->that;
  return NULL;
}

template <class T>
T *TDoubleListImp<T>::LastThat(CondFunc cond, void *args)
{
  for (element *ptr = tail; ptr; ptr = ptr->prev)
    if (cond(ptr->that, args)) return &ptr->that;
  return NULL;
}

template <class T>
void TDoubleListImp<T>::DetachAtHead()
{
  if (head)
  {
    element *newhead = head->next;
    delete head;
    head = newhead;
    if (newhead == NULL) tail = NULL;
    elementcount--;
  }
}

template <class T>
void TDoubleListImp<T>::DetachAtTail()
{
  if (tail)
  {
    element *newtail = tail->prev;
    delete tail;
    tail = newtail;
    if (newtail == NULL) head = NULL;
    elementcount--;
  }
}

#endif

