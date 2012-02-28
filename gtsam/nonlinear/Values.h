/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file Values.h
 * @author Richard Roberts
 *
 * @brief A non-templated config holding any types of Manifold-group elements
 *
 *  Detailed story:
 *  A values structure is a map from keys to values. It is used to specify the value of a bunch
 *  of variables in a factor graph. A Values is a values structure which can hold variables that
 *  are elements on manifolds, not just vectors. It then, as a whole, implements a aggregate type
 *  which is also a manifold element, and hence supports operations dim, retract, and localCoordinates.
 */

#pragma once

#include <string>
#include <utility>

#include <boost/pool/pool_alloc.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ptr_container/serialize_ptr_map.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/lambda/lambda.hpp>

#include <gtsam/base/Value.h>
#include <gtsam/base/FastMap.h>
#include <gtsam/linear/VectorValues.h>
#include <gtsam/nonlinear/Ordering.h>

namespace gtsam {

  // Forward declarations
  class ValueCloneAllocator;
  class ValueAutomaticCasting;

/**
  * A non-templated config holding any types of Manifold-group elements.  A
  * values structure is a map from keys to values. It is used to specify the
  * value of a bunch of variables in a factor graph. A Values is a values
  * structure which can hold variables that are elements on manifolds, not just
  * vectors. It then, as a whole, implements a aggregate type which is also a
  * manifold element, and hence supports operations dim, retract, and
  * localCoordinates.
  */
  class Values {

  private:

    // Internally we store a boost ptr_map, with a ValueCloneAllocator (defined
    // below) to clone and deallocate the Value objects, and a boost
    // fast_pool_allocator to allocate map nodes.  In this way, all memory is
    // allocated in a boost memory pool.
    typedef boost::ptr_map<
        Key,
        Value,
        std::less<Key>,
        ValueCloneAllocator,
        boost::fast_pool_allocator<std::pair<const Key, void*> > > KeyValueMap;

    // The member to store the values, see just above
    KeyValueMap values_;

    // Types obtained by iterating
    typedef KeyValueMap::const_iterator::value_type ConstKeyValuePtrPair;
    typedef KeyValueMap::iterator::value_type KeyValuePtrPair;

  public:

    /// A shared_ptr to this class
    typedef boost::shared_ptr<Values> shared_ptr;

    /// A key-value pair, which you get by dereferencing iterators
    struct KeyValuePair {
      const Key key; ///< The key
      Value& value;  ///< The value

      KeyValuePair(Key _key, Value& _value) : key(_key), value(_value) {}
    };

    /// A key-value pair, which you get by dereferencing iterators
    struct ConstKeyValuePair {
      const Key key; ///< The key
      const Value& value;  ///< The value

      ConstKeyValuePair(Key _key, const Value& _value) : key(_key), value(_value) {}
      ConstKeyValuePair(const KeyValuePair& kv) : key(kv.key), value(kv.value) {}
    };

    /// Mutable forward iterator, with value type KeyValuePair
    typedef boost::transform_iterator<
        boost::function1<KeyValuePair, const KeyValuePtrPair&>, KeyValueMap::iterator> iterator;

    /// Const forward iterator, with value type ConstKeyValuePair
    typedef boost::transform_iterator<
        boost::function1<ConstKeyValuePair, const ConstKeyValuePtrPair&>, KeyValueMap::const_iterator> const_iterator;

    /// Mutable reverse iterator, with value type KeyValuePair
    typedef boost::transform_iterator<
        boost::function1<KeyValuePair, const KeyValuePtrPair&>, KeyValueMap::reverse_iterator> reverse_iterator;

    /// Const reverse iterator, with value type ConstKeyValuePair
    typedef boost::transform_iterator<
        boost::function1<ConstKeyValuePair, const ConstKeyValuePtrPair&>, KeyValueMap::const_reverse_iterator> const_reverse_iterator;

  private:
    template<class ValueType>
    struct _KeyValuePair {
      const Key key; ///< The key
      ValueType& value;  ///< The value

      _KeyValuePair(Key _key, ValueType& _value) : key(_key), value(_value) {}
    };

    template<class ValueType>
    struct _ConstKeyValuePair {
      const Key key; ///< The key
      const ValueType& value;  ///< The value

      _ConstKeyValuePair(Key _key, const ValueType& _value) : key(_key), value(_value) {}
      _ConstKeyValuePair(const _KeyValuePair<ValueType>& rhs) : key(rhs.key), value(rhs.value) {}
    };

  public:
    template<class ValueType = Value>
    class Filtered : public boost::transformed_range<
    _KeyValuePair<ValueType>(*)(Values::KeyValuePair key_value),
    const boost::filtered_range<
    boost::function<bool(const ConstKeyValuePair&)>,
    const boost::iterator_range<iterator> > > {
    public:
      typedef _KeyValuePair<ValueType> KeyValuePair;

      /** Returns the number of values in this view */
      size_t size() const {
      	typename Base::iterator it = this->begin();
      	size_t i = 0;
      	for (; it!=this->end(); ++it)
      		++i;
      	return i;
      }

    private:

      typedef boost::transformed_range<
    			KeyValuePair(*)(Values::KeyValuePair key_value),
    			const boost::filtered_range<
    			boost::function<bool(const Values::ConstKeyValuePair&)>,
    			const boost::iterator_range<iterator> > > Base;

      Filtered(const Base& base) : Base(base) {}

      friend class Values;
    };

    template<class ValueType = Value>
    class ConstFiltered : public boost::transformed_range<
    _ConstKeyValuePair<ValueType>(*)(Values::ConstKeyValuePair key_value),
    const boost::filtered_range<
    boost::function<bool(const ConstKeyValuePair&)>,
    const boost::iterator_range<const_iterator> > > {
    public:
      typedef _ConstKeyValuePair<ValueType> KeyValuePair;

      /** Conversion from Filtered to ConstFiltered */
      ConstFiltered(const Filtered<ValueType>& rhs) : Base(
          boost::adaptors::transform(
              boost::adaptors::filter(
                  boost::make_iterator_range(
                      const_iterator(rhs.begin().base().base().base(), &make_const_deref_pair),
                      const_iterator(rhs.end().base().base().base(), &make_const_deref_pair)),
                  rhs.begin().base().predicate()),
              &castHelper<const ValueType, _ConstKeyValuePair<ValueType>, ConstKeyValuePair>)) {}

      /** Returns the number of values in this view */
      size_t size() const {
      	typename Base::const_iterator it = this->begin();
      	size_t i = 0;
      	for (; it!=this->end(); ++it)
      		++i;
      	return i;
      }

    private:

      typedef boost::transformed_range<
          KeyValuePair(*)(Values::ConstKeyValuePair key_value),
          const boost::filtered_range<
          boost::function<bool(const Values::ConstKeyValuePair&)>,
          const boost::iterator_range<const_iterator> > > Base;

      ConstFiltered(const Base& base) : Base(base) {}

      friend class Values;
    };

    /** Default constructor creates an empty Values class */
    Values() {}

    /** Copy constructor duplicates all keys and values */
    Values(const Values& other);

    /** Constructor from a Filtered view copies out all values */
    template<class ValueType>
    Values(const Filtered<ValueType>& view) {
    	BOOST_FOREACH(const typename Filtered<ValueType>::KeyValuePair& key_value, view) {
        Key key = key_value.key;
        insert(key, key_value.value);
      }
    }

    /** Constructor from Const Filtered view */
    template<class ValueType>
    Values(const ConstFiltered<ValueType>& view) {
    	BOOST_FOREACH(const typename ConstFiltered<ValueType>::KeyValuePair& key_value, view) {
        Key key = key_value.key;
        insert(key, key_value.value);
      }
    }

    /// @name Testable
    /// @{

    /** print method for testing and debugging */
    void print(const std::string& str = "", const KeyFormatter& keyFormatter = DefaultKeyFormatter) const;

    /** Test whether the sets of keys and values are identical */
    bool equals(const Values& other, double tol=1e-9) const;

    /// @}

    /** Retrieve a variable by key \c j.  The type of the value associated with
     * this key is supplied as a template argument to this function.
     * @param j Retrieve the value associated with this key
     * @tparam Value The type of the value stored with this key, this method
     * throws DynamicValuesIncorrectType if this requested type is not correct.
     * @return A const reference to the stored value
     */
    template<typename ValueType>
    const ValueType& at(Key j) const;

#if 0
    /** Retrieve a variable by key \c j.  This non-templated version returns a
     * special ValueAutomaticCasting object that may be assigned to the proper
     * value.
     * @param j Retrieve the value associated with this key
     * @return A ValueAutomaticCasting object that may be assignmed to a Value
     * of the proper type.
     */
    ValueAutomaticCasting at(Key j) const;
#endif

#if 0
    /** operator[] syntax for at(Key j) */
    ValueAutomaticCasting operator[](Key j) const;
#endif

    /** Check if a value exists with key \c j.  See exists<>(Key j)
     * and exists(const TypedKey& j) for versions that return the value if it
     * exists. */
    bool exists(Key j) const;

    /** Check if a value with key \c j exists, returns the value with type
     * \c Value if the key does exist, or boost::none if it does not exist.
     * Throws DynamicValuesIncorrectType if the value type associated with the
     * requested key does not match the stored value type. */
    template<typename ValueType>
    boost::optional<const ValueType&> exists(Key j) const;

    /** The number of variables in this config */
    size_t size() const { return values_.size(); }

    /** whether the config is empty */
    bool empty() const { return values_.empty(); }

    /** Get a zero VectorValues of the correct structure */
    VectorValues zeroVectors(const Ordering& ordering) const;

  private:
    static ConstKeyValuePair make_const_deref_pair(const KeyValueMap::const_iterator::value_type& key_value) {
      return ConstKeyValuePair(key_value.first, *key_value.second); }
    static KeyValuePair make_deref_pair(const KeyValueMap::iterator::value_type& key_value) {
      return KeyValuePair(key_value.first, *key_value.second); }

  public:
    const_iterator begin() const { return boost::make_transform_iterator(values_.begin(), &make_const_deref_pair); }
    const_iterator end() const { return boost::make_transform_iterator(values_.end(), &make_const_deref_pair); }
    iterator begin() { return boost::make_transform_iterator(values_.begin(), &make_deref_pair); }
    iterator end() { return boost::make_transform_iterator(values_.end(), &make_deref_pair); }
    const_reverse_iterator rbegin() const { return boost::make_transform_iterator(values_.rbegin(), &make_const_deref_pair); }
    const_reverse_iterator rend() const { return boost::make_transform_iterator(values_.rend(), &make_const_deref_pair); }
    reverse_iterator rbegin() { return boost::make_transform_iterator(values_.rbegin(), &make_deref_pair); }
    reverse_iterator rend() { return boost::make_transform_iterator(values_.rend(), &make_deref_pair); }

    /// @name Manifold Operations
    /// @{

    /** Add a delta config to current config and returns a new config */
    Values retract(const VectorValues& delta, const Ordering& ordering) const;

    /** Get a delta config about a linearization point c0 (*this) */
    VectorValues localCoordinates(const Values& cp, const Ordering& ordering) const;

    /** Get a delta config about a linearization point c0 (*this) */
    void localCoordinates(const Values& cp, const Ordering& ordering, VectorValues& delta) const;

    ///@}

    /** Add a variable with the given j, throws KeyAlreadyExists<J> if j is already present */
    void insert(Key j, const Value& val);

    /** Add a set of variables, throws KeyAlreadyExists<J> if a key is already present */
    void insert(const Values& values);

    /** single element change of existing element */
    void update(Key j, const Value& val);

    /** update the current available values without adding new ones */
    void update(const Values& values);

    /** Remove a variable from the config, throws KeyDoesNotExist<J> if j is not present */
    void erase(Key j);

    /**
     * Returns a set of keys in the config
     * Note: by construction, the list is ordered
     */
    FastList<Key> keys() const;

    /** Replace all keys and variables */
    Values& operator=(const Values& rhs);

    /** Remove all variables from the config */
    void clear() { values_.clear(); }

    /** Create an array of variable dimensions using the given ordering (\f$ O(n) \f$) */
    std::vector<size_t> dims(const Ordering& ordering) const;

    /** Compute the total dimensionality of all values (\f$ O(n) \f$) */
    size_t dim() const;

    /**
     * Generate a default ordering, simply in key sort order.  To instead
     * create a fill-reducing ordering, use
     * NonlinearFactorGraph::orderingCOLAMD().  Alternatively, you may permute
     * this ordering yourself (as orderingCOLAMD() does internally).
     */
    Ordering::shared_ptr orderingArbitrary(Index firstVar = 0) const;

    /**
     * Return a filtered view of this Values class, without copying any data.
     * When iterating over the filtered view, only the key-value pairs
     * with a key causing \c filterFcn to return \c true are visited.  Because
     * the object Filtered<Value> returned from filter() is only a
     * <emph>view</emph> the original Values object must not be deallocated or
     * go out of scope as long as the view is needed.
     * @param filterFcn The function that determines which key-value pairs are
     * included in the filtered view, for which this function returns \c true
     * on their keys.
     * @return A filtered view of the original Values class, which references
     * the original Values class.
     */
    Filtered<Value>
    filter(const boost::function<bool(Key)>& filterFcn) {
      return filter<Value>(filterFcn);
    }

    /**
     * Return a filtered view of this Values class, without copying any data.
     * In this templated version, only key-value pairs whose value matches the
     * template argument \c ValueType and whose key causes the function argument
     * \c filterFcn to return true are visited when iterating over the filtered
     * view.  Because the object Filtered<Value> returned from filter() is only
     * a <emph>view</emph> the original Values object must not be deallocated or
     * go out of scope as long as the view is needed.
     * @tparam ValueType The type that the value in a key-value pair must match
     * to be included in the filtered view.  Currently, base classes are not
     * resolved so the type must match exactly, except if ValueType = Value, in
     * which case no type filtering is done.
     * @param filterFcn The function that determines which key-value pairs are
     * included in the filtered view, for which this function returns \c true
     * on their keys (default:  always return true so that filter() only
     * filters by type, matching \c ValueType).
     * @return A filtered view of the original Values class, which references
     * the original Values class.
     */
    template<class ValueType>
    Filtered<ValueType>
    filter(const boost::function<bool(Key)>& filterFcn = (boost::lambda::_1, true)) {
      return boost::adaptors::transform(
          boost::adaptors::filter(
              boost::make_iterator_range(begin(), end()),
              boost::function<bool(const ConstKeyValuePair&)>(
                  boost::bind(&filterHelper<ValueType>, filterFcn, _1))),
          &castHelper<ValueType, _KeyValuePair<ValueType>, KeyValuePair>);
    }

    /**
     * Return a filtered view of this Values class, without copying any data.
     * When iterating over the filtered view, only the key-value pairs
     * with a key causing \c filterFcn to return \c true are visited.  Because
     * the object Filtered<Value> returned from filter() is only a
     * <emph>view</emph> the original Values object must not be deallocated or
     * go out of scope as long as the view is needed.
     * @param filterFcn The function that determines which key-value pairs are
     * included in the filtered view, for which this function returns \c true
     * on their keys.
     * @return A filtered view of the original Values class, which references
     * the original Values class.
     */
    ConstFiltered<Value>
    filter(const boost::function<bool(Key)>& filterFcn) const {
      return filter<Value>(filterFcn);
    }

    /**
     * Return a filtered view of this Values class, without copying any data.
     * In this templated version, only key-value pairs whose value matches the
     * template argument \c ValueType and whose key causes the function argument
     * \c filterFcn to return true are visited when iterating over the filtered
     * view.  Because the object Filtered<Value> returned from filter() is only
     * a <emph>view</emph> the original Values object must not be deallocated or
     * go out of scope as long as the view is needed.
     * @tparam ValueType The type that the value in a key-value pair must match
     * to be included in the filtered view.  Currently, base classes are not
     * resolved so the type must match exactly, except if ValueType = Value, in
     * which case no type filtering is done.
     * @param filterFcn The function that determines which key-value pairs are
     * included in the filtered view, for which this function returns \c true
     * on their keys.
     * @return A filtered view of the original Values class, which references
     * the original Values class.
     */
    template<class ValueType>
    ConstFiltered<ValueType>
    filter(const boost::function<bool(Key)>& filterFcn = (boost::lambda::_1, true)) const {
      return boost::adaptors::transform(
          boost::adaptors::filter(
              boost::make_iterator_range(begin(), end()),
              boost::function<bool(const ConstKeyValuePair&)>(
                  boost::bind(&filterHelper<ValueType>, filterFcn, _1))),
          &castHelper<const ValueType, _ConstKeyValuePair<ValueType>, ConstKeyValuePair>);
    }

  private:
    // Filters based on ValueType (if not Value) and also based on the user-
    // supplied \c filter function.
    template<class ValueType>
    static bool filterHelper(const boost::function<bool(Key)> filter, const ConstKeyValuePair& key_value) {
      // Filter and check the type
      return filter(key_value.key) && (typeid(ValueType) == typeid(key_value.value) || typeid(ValueType) == typeid(Value));
    }

    // Cast to the derived ValueType
    template<class ValueType, class CastedKeyValuePairType, class KeyValuePairType>
    static CastedKeyValuePairType castHelper(KeyValuePairType key_value) {
      // Static cast because we already checked the type during filtering
      return CastedKeyValuePairType(key_value.key, static_cast<ValueType&>(key_value.value));
    }

    /** Serialization function */
  	friend class boost::serialization::access;
  	template<class ARCHIVE>
  	void serialize(ARCHIVE & ar, const unsigned int version) {
  		ar & BOOST_SERIALIZATION_NVP(values_);
  	}

  };

  /* ************************************************************************* */
  class ValuesKeyAlreadyExists : public std::exception {
  protected:
    const Key key_; ///< The key that already existed

  private:
    mutable std::string message_;

  public:
    /// Construct with the key-value pair attemped to be added
    ValuesKeyAlreadyExists(Key key) throw() :
      key_(key) {}

    virtual ~ValuesKeyAlreadyExists() throw() {}

    /// The duplicate key that was attemped to be added
    Key key() const throw() { return key_; }

    /// The message to be displayed to the user
    virtual const char* what() const throw();
  };

  /* ************************************************************************* */
  class ValuesKeyDoesNotExist : public std::exception {
  protected:
    const char* operation_; ///< The operation that attempted to access the key
    const Key key_; ///< The key that does not exist

  private:
    mutable std::string message_;

  public:
    /// Construct with the key that does not exist in the values
    ValuesKeyDoesNotExist(const char* operation, Key key) throw() :
      operation_(operation), key_(key) {}

    virtual ~ValuesKeyDoesNotExist() throw() {}

    /// The key that was attempted to be accessed that does not exist
    Key key() const throw() { return key_; }

    /// The message to be displayed to the user
    virtual const char* what() const throw();
  };

  /* ************************************************************************* */
  class ValuesIncorrectType : public std::exception {
  protected:
    const Key key_; ///< The key requested
    const std::type_info& storedTypeId_;
    const std::type_info& requestedTypeId_;

  private:
    mutable std::string message_;

  public:
    /// Construct with the key that does not exist in the values
    ValuesIncorrectType(Key key,
        const std::type_info& storedTypeId, const std::type_info& requestedTypeId) throw() :
      key_(key), storedTypeId_(storedTypeId), requestedTypeId_(requestedTypeId) {}

    virtual ~ValuesIncorrectType() throw() {}

    /// The key that was attempted to be accessed that does not exist
    Key key() const throw() { return key_; }

    /// The typeid of the value stores in the Values
    const std::type_info& storedTypeId() const { return storedTypeId_; }

    /// The requested typeid
    const std::type_info& requestedTypeId() const { return requestedTypeId_; }

    /// The message to be displayed to the user
    virtual const char* what() const throw();
  };

  /* ************************************************************************* */
  class DynamicValuesMismatched : public std::exception {

  public:
    DynamicValuesMismatched() throw() {}

    virtual ~DynamicValuesMismatched() throw() {}

    virtual const char* what() const throw() {
      return "The Values 'this' and the argument passed to Values::localCoordinates have mismatched keys and values";
    }
  };

}

#include <gtsam/nonlinear/Values-inl.h>
