#include <bandit/bandit.h>

using namespace bandit;
using namespace snowhouse;

go_bandit([]() {
    describe("1+1", []() {
        it("yields 2", []() {
            AssertThat(1 + 1, Equals(2));
        });
    });
});

