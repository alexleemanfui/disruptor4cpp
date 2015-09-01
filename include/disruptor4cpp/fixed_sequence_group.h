/*
Copyright (c) 2015, Alex Man-fui Lee
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of disruptor4cpp nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DISRUPTOR4CPP_FIXED_SEQUENCE_GROUP_H_
#define DISRUPTOR4CPP_FIXED_SEQUENCE_GROUP_H_

#include <climits>
#include <cstdint>
#include <vector>

#include "sequence.h"
#include "utils/util.h"

namespace disruptor4cpp
{
	template <typename TSequence = sequence>
	class fixed_sequence_group
	{
	public:
		static fixed_sequence_group<TSequence> create(const std::vector<const TSequence*>& sequences)
		{
			fixed_sequence_group<TSequence> group;
			group.sequences_.insert(group.sequences_.begin(), sequences.begin(), sequences.end());
			return group;
		}

		static fixed_sequence_group<TSequence> create(const std::vector<TSequence*>& sequences)
		{
			fixed_sequence_group<TSequence> group;
			group.sequences_.insert(group.sequences_.begin(), sequences.begin(), sequences.end());
			return group;
		}

		static fixed_sequence_group<TSequence> create(const TSequence& sequence)
		{
			fixed_sequence_group<TSequence> group;
			group.sequences_.push_back(&sequence);
			return group;
		}

		fixed_sequence_group() = default;
		~fixed_sequence_group() = default;

		int64_t get() const
		{
			return sequences_.size() == 1 ? sequences_[0]->get()
				: util::get_minimum_sequence(sequences_);
		}

	private:
		std::vector<const TSequence*> sequences_;
	};
}

#endif
