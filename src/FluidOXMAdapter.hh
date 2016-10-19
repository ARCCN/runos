#include <cstdint>
#include <cstddef>
#include <fluid/of13/of13match.hh>

#include "oxm/field.hh"
#include "oxm/field_set_fwd.hh"

namespace runos {

class FluidOXMAdapter : public fluid_msg::of13::OXMTLV {
    oxm::field<> field_;
public:
    FluidOXMAdapter(const oxm::field<> f);
    OXMTLV& operator=(const OXMTLV& other) override;
    FluidOXMAdapter* clone() const override;

    bool equals(const OXMTLV& other) override;

    size_t pack(uint8_t* buffer) override;
    fluid_msg::of_error unpack(uint8_t* buffer) override;
};

fluid_msg::of13::Match make_of_match(const oxm::field_set &match);

} // namespace runos
