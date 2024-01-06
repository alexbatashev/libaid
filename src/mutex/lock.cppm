module;

export module aid.mutex:lock;

export namespace aid {
template <typename T>
concept lockable = requires(T m) {
  { m.lock() };
  { m.unlock() };
};

template <typename T>
concept rw_lockable = lockable<T> && requires(T m) {
  { m.lock_shared() };
  { m.unlock_shared() };
  { m.upgrade() };
};

/// RAII-style read lock.
///
/// Unlike std::shared_lock this class provides aims to upgrade reader lock to
/// writer lock, which in some cases can improve performance.
template <rw_lockable Mutex>
class shared_lock {
public:
  shared_lock() = default;

  explicit shared_lock(Mutex &m) noexcept(Mutex::is_noexcept) { acquire(m); }

  void acquire(Mutex &m) noexcept(Mutex::is_noexcept) {
    mMutex = &m;
    m.lock_shared();
  }

  void unlock() noexcept(Mutex::is_noexcept) {
    if (mIsWriter)
      mMutex->unlock();
    else
      mMutex->unlock_shared();
    mMutex = nullptr;
    mIsWriter = false;
  }

  void upgrade() noexcept(Mutex::is_noexcept) {
    if (!mIsWriter) {
      mIsWriter = true;
      mMutex->upgrade();
    }
  }

  ~shared_lock() {
    if (mMutex)
      unlock();
  }

private:
  Mutex *mMutex;
  bool mIsWriter = false;
};

template <rw_lockable Mutex>
shared_lock(Mutex &) -> shared_lock<Mutex>;
} // namespace aid