module JSONParser where

import Control.Arrow

data JSON
  = JSONUndef
  | JSONBool Bool
  | JSONNum Double
  | JSONString String
  | JSONObject [(String, JSON)]
  | JSONArray [JSON] 
  deriving (Show, Eq)
  

parseJSON :: String -> (JSON, String)
parseJSON "" = error "Nothing here"
parseJSON a@(x:xs) = case x of
  ' ' -> parseJSON xs
  '\t' -> parseJSON xs
  '\n' -> parseJSON xs
  '[' -> first JSONArray $ parseArray xs
  '{' -> first JSONObject $ parseObject xs
  '"' -> first JSONString $ parseString xs
  _ -> case a of
    ('t':'r':'u':'e':xs) -> (JSONBool True, xs)
    ('f':'a':'l':'s':'e':xs) -> (JSONBool False, xs)
    ('u':'n':'d':'e':'f':'i':'n':'e':'d':xs) -> (JSONUndef, xs)
    _ -> case reads a :: [(Double, String)] of
      [] -> error "unparsable"
      [(c, d)] -> (JSONNum c, d)
      (_:_:_) -> error "wtf?"
      
parseArray :: String -> ([JSON], String)
parseArray a@(x:xs) = case x of
  ' ' -> parseJSONArray xs
  '\t' -> parseJSONArray xs
  '\n' -> parseJSONArray xs
  ']' -> ([], xs)
  _ -> let (o, r) = parseJSON a in
    
  

parseObject :: a
parseObject = undefined

parseString :: a
parseString = undefined
