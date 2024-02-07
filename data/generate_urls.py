import random
import string

def generate_random_url(https=True, domain_length=10, path_length=15):
    protocol = "https" if https else "http"
    domain_name = ''.join(random.choices(string.ascii_lowercase, k=domain_length))
    tld = random.choice(['com', 'net', 'org', 'io', 'ru', 'to'])
    path = ''.join(random.choices(string.ascii_letters + string.digits, k=path_length))
    return f"{protocol}://{domain_name}.{tld}/{path}"

def generate_urls_file(file_name, number_of_urls, domain_length=10, path_length=15):
    with open(file_name, 'w') as file:
        for _ in range(number_of_urls):
            url = generate_random_url(domain_length=domain_length, path_length=path_length)
            file.write(url + '\n')

# Example usage
file_name = 'random_urls.txt'
number_of_urls = 10000000  # Adjust the number of URLs here
domain_length = 30  # Adjust the length of the domain name here
path_length = 100  # Adjust the length of the path here

generate_urls_file(file_name, number_of_urls, domain_length, path_length)
