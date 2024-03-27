from coinbase import jwt_generator

pipe_path = "/tmp/jwt_pipe"
secret_key_fpath = "./coinbase_secret_key.txt"
api_key_fpath = "./coinbase_api_key.txt"


def main():
    with open(secret_key_fpath, "rb") as f:
        api_secret = f.read().decode("utf-8").replace('\\n', '\n')

    with open(api_key_fpath, "rb") as f:
        api_key = f.read().decode("utf-8")
        

    jwt = jwt_generator.build_ws_jwt(api_key, api_secret)

    with open(pipe_path, "w") as pipe:
        pipe.write(jwt)
        print(f"JWT written to pipe: {jwt}")

if __name__ == "__main__":
    main()