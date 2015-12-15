
#include"../util.hpp"

class JSONException : public TraceCeption
{
public:
  explicit JSONException(std::string errormsg)
    : TraceCeption(TextPos(nullptr), errormsg)
  {}
  JSONException(TextPos pos, std::string errormsg)
    : TraceCeption(pos, errormsg)
  {}
};
