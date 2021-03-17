//  Copyright 2021 Herald Project Contributors
//  SPDX-License-Identifier: Apache-2.0
//

#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include "sampling.h"

#include <variant>
#include <array>

namespace herald {
namespace analysis {

using namespace sampling;

/// \brief Base Interface definition for classes that receive newSample callbacks from AnalysisRunner
struct AnalysisDelegate {
  AnalysisDelegate() = default;
  virtual ~AnalysisDelegate() = default;

  template <typename ValT>
  void newSample(SampledID sampled, Sample<ValT> sample);

  template <typename RunnerT>
  void setDestination(RunnerT& runner);
};

/// \brief Manages a set of lists for a particular Sample Value Type
template <typename ValT, std::size_t Size, std::size_t MaxLists>
struct ListManager {
  using value_type = ValT;

  ListManager() noexcept : lists(), nextPos(0) {};
  ListManager(ListManager&& other) : lists(std::move(other.lists)), nextPos(other.nextPos) {}; // move ctor
  ~ListManager() = default;

  SampleList<Sample<ValT>,Size>& list(const SampledID sampled) {
    for (auto& entry : lists) {
      if (entry.key() == sampled) {
        return entry.value();
      }
    }
    auto& val = lists[nextPos++];
    return val.emplace(sampled);
  }

  void remove(const SampledID listFor) {
    // lists.erase(listFor);
    // TODO delete value and re-use its allocation space
  }

  const std::size_t size() const {
    return nextPos;
  }

private:
  template <typename Key>
  struct ListManagerEntry {
    ListManagerEntry() : mKey(), mValue() {}
    ~ListManagerEntry() = default;

    SampleList<Sample<ValT>,Size>& emplace(const Key k) {
      mKey = k;
      return value();
    }

    const Key key() const {
      return mKey;
    }

    SampleList<Sample<ValT>,Size>& value() {
      return mValue;
    }

  private:
    Key mKey;
    SampleList<Sample<ValT>,Size> mValue;
  };

  std::array<ListManagerEntry<SampledID>,MaxLists> lists;
  int nextPos;
};
// template <typename ValT, std::size_t Size>
// struct ListManager {
//   using value_type = ValT;

//   ListManager() = default;
//   ~ListManager() = default;

//   SampleList<Sample<ValT>,Size>& list(const SampledID sampled) {
//     auto iter = lists.try_emplace(sampled).first;
//     return lists.at(sampled);
//   }

//   void remove(const SampledID listFor) {
//     lists.erase(listFor);
//   }

//   const std::size_t size() const {
//     return lists.size();
//   }

// private:
//   std::map<SampledID,SampleList<Sample<ValT>,Size>> lists;
// };

/// \brief A fixed size set that holds exactly one instance of the std::variant for each
/// of the specified ValTs value types.
template <typename... ValTs>
struct VariantSet {
  static constexpr std::size_t Size = sizeof...(ValTs);

  VariantSet() : variants() {
    createInstances<ValTs...>(0);
  }; // Instantiate each type instance in the array
  ~VariantSet() = default;

  /// CAN THROW std::bad_variant_access
  template <typename ValT>
  ValT& get() {
    for (auto& v : variants) {
      if (auto pval = std::get_if<ValT>(&v)) {
        return *pval;
      }
    }
    throw std::bad_variant_access();
  }

  const std::size_t size() const {
    return variants.size();
  }

private:
  std::array<std::variant<ValTs...>,Size> variants;
  template <typename LastT>
  void createInstances(int pos) {
    variants[pos].template emplace<LastT>();
  }

  template <typename FirstT, typename SecondT, typename... RestT>
  void createInstances(int pos) {
    variants[pos].template emplace<FirstT>();
    createInstances<SecondT,RestT...>(pos + 1);
  }
};

/// The below is an example ValueSource...
/// template <typename ValT>
/// struct ExValueSource {
///   using value_type = ValT;
/// 
///   template <typename RunnerT>
///   void setDestination(RunnerT& runner) {
///     // save reference
///   }
/// 
///   // At some predetermined point external to the analyser runner
///   // this ValueSource will call runner.newSample(Sample<ValT> sample)
/// };

/// \brief Manages all sample lists, sources, sinks, and analysis instances for all data generated within a system
///
/// This class can be used 'live' against real sensors, or statically with reference data. 
/// This is achieved by ensuring the run(Date) method takes in the Date for the time of evaluation rather
/// than using the current Date.
// template <typename... SourceTypes>
// struct AnalysisRunner {
//   // using valueTypes = (typename SourceTypes::value_type)...;

//   AnalysisRunner(/*SourceTypes&... sources*/) : lists(), notifiables() {
//     // registerRunner(sources...);
//   }
//   ~AnalysisRunner() = default;

//   /// We are an analysis delegate ourselves - this is used by Source types, and by producers (analysis runners)
//   template <typename ValT, std::size_t Size>
//   void newSample(SampledID sampled, sampling::Sample<ValT> sample) {
//     // incoming sample. Pass to correct list
//     lists.template get<ListManager<ValT,Size>>().list(sampled)->second.push(sample);
//   }

//   /// Run the relevant analyses given the current time point
//   void run(Date timeNow) {
//     // call analyse(dateNow,srcList,dstDelegate) for all delegates with the correct list each, for each sampled
//     // TODO performance enhancement - 'dirty' sample lists only (ones with new data)
//     for (auto& listManager : lists) {
//       using ValT = listManager::value_type;
//       for (auto& delegate : notifiables) {
//         // if constexpr (std::is_same_v<ValT,delegate::value_type>) { // SHOULD BE RUNNERS NOT DELEGATES
//           //delegate.
//         // }
//       }
//     }
//   }

//   void add(std::shared_ptr<AnalysisDelegate> delegate) {
//     notifiables.push_back(delegate);
//   }

//   template <typename AnalyserT>
//   void addAnalyser(AnalyserT& analyser) {
//     // TODO fill this out and call its analyse() function when required
//   }

//   // /// callback from analysis data source
//   // template <typename ValT>
//   // SampleList<Sample<ValT>>& list(const SampledID sampled) {
//   //   return lists.get<ValT>().list(sampled);
//   // }

// private:
//   // TODO make sizes a parameterised list derived from template parameters
//   VariantSet<ListManager<SourceTypes,25>...> lists; // exactly one list manager per value type
//   std::vector<std::shared_ptr<AnalysisDelegate>> notifiables; // more than one delegate per value type
//   //std::vector< // runners

//   // template <typename FirstT>
//   // void registerRunner(FirstT first) {
//   //   first.setDestination(*this);
//   // }

//   // template <typename FirstT, typename SecondT, typename... RestT>
//   // void registerRunner(FirstT first,SecondT second,RestT... rest) {
//   //   first.setDestination(*this);
//   //   registerRunner(second,rest...);
//   // }
// };

}
}

#endif